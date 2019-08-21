#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast.hpp"
#include "../../include/altacore/util.hpp"

bool AltaCore::DET::CastComponent::operator ==(const CastComponent& other) const {
  if (type != other.type) return false;
  if (special != other.special) return false;
  if (!!method != !!other.method) return false;
  if (method && method->id != other.method->id) return false;
  if (!!target != !!other.target) return false;
  if (target && !(*target == *other.target)) return false;
  if (!!klass != !!other.klass) return false;
  if (klass && klass->id != other.klass->id) return false;
  if (!!via != !!other.via) return false;
  if (via && !(*via == *other.via)) return false;
  return true;
};

const AltaCore::DET::NodeType AltaCore::DET::Type::nodeType() {
  return NodeType::Type;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Type::clone() {
  return std::make_shared<Type>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Type::deepClone() {
  return clone();
};

std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::copy() const {
  return std::make_shared<Type>(*this);
};

std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::getUnderlyingType(AltaCore::DH::ExpressionNode* expression) {
  using Modifier = AST::TypeModifierFlag;

  if (auto intLit = dynamic_cast<DH::IntegerLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Integer, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto varDef = dynamic_cast<DH::VariableDefinitionExpression*>(expression)) {
    return std::dynamic_pointer_cast<Type>(varDef->variable->type->clone())->reference();
  } else if (auto assign = dynamic_cast<DH::AssignmentExpression*>(expression)) {
    return getUnderlyingType(assign->target.get())->reference();
  } else if (auto fetch = dynamic_cast<DH::Fetch*>(expression)) {
    if (!fetch->narrowedTo) {
      if (fetch->readAccessor) {
        return fetch->readAccessor->returnType;
      }
      throw std::runtime_error("the given fetch has not been narrowed. either narrow it or use `AltaCore::DET::Type::getUnderlyingTypes` instead");
    }
    return getUnderlyingType(fetch->narrowedTo);
  } else if (auto boolean = dynamic_cast<DH::BooleanLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Bool, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto binOp = dynamic_cast<DH::BinaryOperation*>(expression)) {
    if (binOp->operatorMethod) {
      return binOp->operatorMethod->returnType;
    }
    if ((uint8_t)binOp->type <= (uint8_t)Shared::OperatorType::BitwiseXor) {
      return getUnderlyingType(binOp->left.get())->destroyReferences();
    } else {
      return std::make_shared<Type>(NativeType::Bool, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
    }
  } else if (auto call = dynamic_cast<DH::FunctionCallExpression*>(expression)) {
    auto type = call->targetType->returnType;
    if (call->maybe) {
      type = std::make_shared<DET::Type>(true, type);
    }
    return type;
  } else if (auto acc = dynamic_cast<DH::Accessor*>(expression)) {
    if (!acc->narrowedTo) {
      if (acc->readAccessor) {
        return acc->readAccessor->returnType;
      }
      throw std::runtime_error("the given accessor has not been narrowed. either narrow it or use `AltaCore::DET::Type::getUnderlyingTypes` instead");
    }
    return getUnderlyingType(acc->narrowedTo);
  } else if (auto str = dynamic_cast<DH::StringLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Byte, std::vector<uint8_t> { (uint8_t)Modifier::Constant | (uint8_t)Modifier::Pointer, (uint8_t)Modifier::Constant });
  } else if (auto cond = dynamic_cast<DH::ConditionalExpression*>(expression)) {
    return getUnderlyingType(cond->primaryResult.get());
  } else if (auto inst = dynamic_cast<DH::ClassInstantiationExpression*>(expression)) {
    if (inst->superclass) {
      return getUnderlyingType(inst->target.get());
    } else {
      auto type = std::make_shared<Type>(inst->klass);
      if (inst->persistent) {
        type = type->reference();
      }
      return type;
    }
  } else if (auto ptr = dynamic_cast<DH::PointerExpression*>(expression)) {
    return getUnderlyingType(ptr->target.get())->destroyReferences()->point();
  } else if (auto deref = dynamic_cast<DH::DereferenceExpression*>(expression)) {
    auto target = getUnderlyingType(deref->target.get());
    if (target->pointerLevel() < 1 && target->isOptional) {
      target = target->optionalTarget->copy();
    } else {
      target = target->follow();
    }
    return target;
  } else if (auto cast = dynamic_cast<DH::CastExpression*>(expression)) {
    return cast->type->type;
  } else if (auto chara = dynamic_cast<DH::CharacterLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Byte, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto subs = dynamic_cast<DH::SubscriptExpression*>(expression)) {
    return getUnderlyingType(subs->target.get())->follow();
  } else if (auto sc = dynamic_cast<DH::SuperClassFetch*>(expression)) {
    return std::make_shared<Type>(sc->superclass, std::vector<uint8_t> { (uint8_t)Modifier::Reference });
  } else if (auto instOf = dynamic_cast<DH::InstanceofExpression*>(expression)) {
    return std::make_shared<Type>(NativeType::Bool, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto unary = dynamic_cast<DH::UnaryOperation*>(expression)) {
    if (unary->operatorMethod) {
      return unary->operatorMethod->returnType;
    }
    if (unary->type == Shared::UOperatorType::Not) {
      return std::make_shared<Type>(NativeType::Bool, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
    } else {
      return getUnderlyingType(unary->target.get())->destroyReferences();
    }
  } else if (auto op = dynamic_cast<DH::SizeofOperation*>(expression)) {
    return std::make_shared<Type>(NativeType::Integer, std::vector<uint8_t> { (uint8_t)Modifier::Constant, (uint8_t)Modifier::Long, (uint8_t)Modifier::Long });
  } else if (auto deci = dynamic_cast<DH::FloatingPointLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Double, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto null = dynamic_cast<DH::NullptrExpression*>(expression)) {
    auto type = std::make_shared<Type>();
    type->modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Pointer);
    return type;
  } else if (auto lambda = dynamic_cast<DH::LambdaExpression*>(expression)) {
    return getUnderlyingType(lambda->function);
  } else if (auto special = dynamic_cast<DH::SpecialFetchExpression*>(expression)) {
    return getUnderlyingType(special->items[0]);
  }

  return nullptr;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::getUnderlyingType(std::shared_ptr<AltaCore::DET::ScopeItem> item) {
  using ItemType = DET::NodeType;
  using Modifier = AST::TypeModifierFlag;
  ItemType itemType = item->nodeType();

  if (itemType == ItemType::Function) {
    auto func = std::dynamic_pointer_cast<Function>(item);
    if (func->genericParameterCount > 0 && func->genericArguments.size() < 1) return nullptr;
    std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> params;
    for (auto& [name, type, isVariable, id]: func->parameters) {
      params.push_back(std::make_tuple(func->isLambda ? "" : name, type, isVariable, id));
    }
    auto type = std::make_shared<Type>(func->returnType, params);
    type->isRawFunction = !func->isLambda;
    type->throws = false;
    func->beganThrowing.listen([=]() {
      type->throws = true;
    });
    if (auto parent = item->parentScope.lock()) {
      if (auto klass = parent->parentClass.lock()) {
        type->isMethod = true;
        type->methodParent = klass;
        type->isAccessor = func->isAccessor;
      }
    }
    return type;
  } else if (itemType == ItemType::Variable) {
    auto var = std::dynamic_pointer_cast<Variable>(item);
    return std::dynamic_pointer_cast<Type>(var->type);
  }
  return nullptr;
};
std::vector<std::shared_ptr<AltaCore::DET::Type>> AltaCore::DET::Type::getUnderlyingTypes(AltaCore::DH::ExpressionNode* expression) {
  using Modifier = AST::TypeModifierFlag;

  if (auto fetch = dynamic_cast<DH::Fetch*>(expression)) {
    std::vector<std::shared_ptr<Type>> types;
    for (auto& item: fetch->items) {
      types.push_back(getUnderlyingType(item));
    }
    return types;
  } else if (auto acc = dynamic_cast<DH::Accessor*>(expression)) {
    std::vector<std::shared_ptr<Type>> types;
    for (auto& item: acc->items) {
      types.push_back(getUnderlyingType(item));
    }
    return types;
  } else if (auto cond = dynamic_cast<DH::ConditionalExpression*>(expression)) {
    return getUnderlyingTypes(cond->primaryResult.get()); // for now; TODO: get a union of both results' types
  }

  auto type = getUnderlyingType(expression);
  if (type == nullptr) return {};
  return { type };
};

std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::reference() const {
  auto other = copy();
  if (other->referenceLevel() < 1) {
    other->modifiers.insert(other->modifiers.begin(), (uint8_t)Shared::TypeModifierFlag::Reference);
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::dereference() const {
  auto other = copy();
  if (other->modifiers.size() > 0) {
    auto& root = other->modifiers.front();
    root &= ~(uint8_t)Shared::TypeModifierFlag::Reference;
    // `pop_back` if the modifier level is now empty
    // why keep around a useless entry in the vector?
    if (root == 0) {
      other->modifiers.erase(other->modifiers.begin());
    }
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::point() const {
  auto other = copy();
  other->modifiers.insert(other->modifiers.begin(), (uint8_t)Shared::TypeModifierFlag::Pointer);
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::follow() const {
  auto other = copy();
  if (other->modifiers.size() > 0) {
    auto& root = other->modifiers.front();
    root &= ~(uint8_t)Shared::TypeModifierFlag::Pointer;
    if (root == 0) {
      other->modifiers.erase(other->modifiers.begin());
    }
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::followBlindly() const {
  auto other = copy();
  if (other->modifiers.size() > 0) {
    auto& root = other->modifiers.front();
    root &= ~((uint8_t)Shared::TypeModifierFlag::Reference | (uint8_t)Shared::TypeModifierFlag::Pointer);
    if (root == 0) {
      other->modifiers.erase(other->modifiers.begin());
    }
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::deconstify(bool full) const {
  auto other = copy();
  if (full) {
    for (size_t i = 0; i < other->modifiers.size(); ++i) {
      auto& mod = other->modifiers[i];
      if (mod & (uint8_t)AltaCore::DET::TypeModifierFlag::Constant) {
        mod &= ~((uint8_t)AltaCore::DET::TypeModifierFlag::Constant);
      }
      if (mod == 0) {
        other->modifiers.erase(other->modifiers.begin() + i);
        --i;
      }
    }
  } else {
    if (other->modifiers.size() > 0) {
      auto& root = other->modifiers.front();
      root &= ~(uint8_t)Shared::TypeModifierFlag::Constant;
      if (root == 0) {
        other->modifiers.erase(other->modifiers.begin());
      }
    }
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::destroyReferences() const {
  auto other = copy();
  while (other->referenceLevel() > 0) {
    other = other->dereference();
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::makeOptional() const {
  return std::make_shared<DET::Type>(true, copy());
}
const size_t AltaCore::DET::Type::indirectionLevel() const {
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Pointer || modLevel & (uint8_t)Shared::TypeModifierFlag::Reference) {
      count++;
    }
  }

  return count;
};
const size_t AltaCore::DET::Type::pointerLevel() const {
  if (referenceLevel() > 0) return destroyReferences()->pointerLevel();
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Pointer) {
      count++;
    }
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Reference) {
      break;
    }
  }

  return count;
};
const size_t AltaCore::DET::Type::referenceLevel() const {
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Reference) {
      count++;
    }
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Pointer) {
      break;
    }
  }

  return count;
};

auto AltaCore::DET::Type::findAllPossibleCasts(std::shared_ptr<Type> from, std::shared_ptr<Type> to, bool manual) -> std::vector<CastPath> {
  std::vector<CastPath> results;

  if (to->isAny) {
    results.push_back({ CastComponent(CastComponentType::Destination) });
  }

  if (*from == *to) {
    results.push_back({ CastComponent(CastComponentType::Destination) });
  }

  if (to->indirectionLevel() > 0) {
    auto nonConstPtrTo = to->deconstify();
    for (size_t i = 0; i < nonConstPtrTo->modifiers.size(); ++i) {
      auto& mod = nonConstPtrTo->modifiers[i];
      if (mod & (uint8_t)AltaCore::DET::TypeModifierFlag::Constant) {
        mod &= ~((uint8_t)AltaCore::DET::TypeModifierFlag::Constant);
      }
      if (mod == 0) {
        nonConstPtrTo->modifiers.erase(nonConstPtrTo->modifiers.begin() + i);
        --i;
      }
    }
    if (*from == *nonConstPtrTo) {
      results.push_back({ CastComponent(CastComponentType::SimpleCoercion, to), CastComponent(CastComponentType::Destination) });
    }
  }

  if (from->pointerLevel() == 0 && to->pointerLevel() == 0 && from->isOptional && to->nativeTypeName == NativeType::Bool) {
    results.push_back({ CastComponent(CastComponentType::Destination, SpecialCastType::OptionalPresent) });
  }

  if (to->pointerLevel() == 0 && to->isOptional && from->pointerLevel() == 1 && from->isAny) {
    results.push_back({ CastComponent(CastComponentType::Destination, SpecialCastType::EmptyOptional) });
  }

  if (from->pointerLevel() == 0 && to->pointerLevel() == 0 && from->isFunction && to->isFunction && from->isRawFunction && !to->isRawFunction) {
    results.push_back({ CastComponent(CastComponentType::Destination, SpecialCastType::WrapFunction) });
  }

  if (
    (
      from->pointerLevel() == 0 &&
      to->pointerLevel() > 0 &&
      (
        from->nativeTypeName == NativeType::Byte ||
        from->nativeTypeName == NativeType::Integer
      )
    ) ||
    (
      to->pointerLevel() == 0 &&
      from->pointerLevel() > 0 &&
      (
        to->nativeTypeName == NativeType::Byte ||
        to->nativeTypeName == NativeType::Integer
      )
    )
  ) {
    results.push_back({ CastComponent(CastComponentType::SimpleCoercion, to), CastComponent(CastComponentType::Destination) });
  }

  if (
    (
      from->pointerLevel() == 0 &&
      to->pointerLevel() > 0 &&
      from->nativeTypeName == NativeType::Bool
    ) ||
    (
      to->pointerLevel() == 0 &&
      from->pointerLevel() > 0 &&
      to->nativeTypeName == NativeType::Bool
    )
  ) {
    results.push_back({ CastComponent(CastComponentType::Destination, SpecialCastType::TestPointer) });
  }

  if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isNative && to->isNative) {
    results.push_back({ CastComponent(CastComponentType::SimpleCoercion, to), CastComponent(CastComponentType::Destination) });
  }

  if (manual && from->pointerLevel() > 0 && to->pointerLevel() > 0) {
    results.push_back({ CastComponent(CastComponentType::SimpleCoercion, to), CastComponent(CastComponentType::Destination) });
  }

  if (from->klass && from->pointerLevel() == 0) {
    for (auto& method: from->klass->toCasts) {
      auto casts = findAllPossibleCasts(method->returnType, to, false);
      for (auto& cast: casts) {
        cast.insert(cast.begin(), CastComponent(CastComponentType::To, method));
      }
      results.insert(results.end(), casts.begin(), casts.end());
    }
  }

  if (to->klass && to->indirectionLevel() == 0) {
    for (auto& method: to->klass->fromCasts) {
      auto casts = findAllPossibleCasts(from, method->parameterVariables[0]->type, false);
      for (auto& cast: casts) {
        cast.insert(cast.end() - 1, CastComponent(CastComponentType::From, method));
      }
      results.insert(results.end(), casts.begin(), casts.end());
    }
  }

  if (from->referenceLevel() > to->referenceLevel()) {
    auto other = from;
    while (other->referenceLevel() > to->referenceLevel()) {
      other = other->dereference();
      auto casts = findAllPossibleCasts(other, to, false);
      for (auto& cast: casts) {
        for (size_t i = from->referenceLevel(); i > other->referenceLevel(); --i) {
          cast.insert(cast.begin(), CastComponent(CastComponentType::Dereference));
        }
      }
      results.insert(results.end(), casts.begin(), casts.end());
    }
  } else if (from->referenceLevel() < to->referenceLevel()) {
    auto other = from;
    while (other->referenceLevel() < to->referenceLevel()) {
      other = other->reference();
      auto casts = findAllPossibleCasts(other, to, false);
      for (auto& cast: casts) {
        for (size_t i = from->referenceLevel(); i < other->referenceLevel(); ++i) {
          cast.insert(cast.begin(), CastComponent(CastComponentType::Reference));
        }
      }
      results.insert(results.end(), casts.begin(), casts.end());
    }
  }

  if (to->indirectionLevel() <= 1 && from->klass && to->klass) {
    for (auto& parent: from->klass->parents) {
      auto casts = findAllPossibleCasts(std::make_shared<Type>(parent), to, false);
      for (auto& cast: casts) {
        cast.insert(cast.begin(), CastComponent(CastComponentType::Upcast, parent));
      }
      results.insert(results.end(), casts.begin(), casts.end());
    }
    if (manual && from->klass && to->klass && from->indirectionLevel() > 0 && to->indirectionLevel() > 0 && to->klass->hasParent(from->klass)) {
      results.push_back({ CastComponent(CastComponentType::Downcast, to->klass) });
    }
  }

  if (to->pointerLevel() == 0) {
    if ((from->pointerLevel() == 0 || !from->isOptional) && to->isOptional && to->indirectionLevel() == 0) {
      if (from->isOptional) {
        auto casts = findAllPossibleCasts(from->optionalTarget, to->optionalTarget, false);
        for (auto& cast: casts) {
          cast.insert(cast.begin(), CastComponent(CastComponentType::Unwrap));
          cast.insert(cast.end() - 1, CastComponent(CastComponentType::Wrap));
        }
        results.insert(results.end(), casts.begin(), casts.end());
      } else {
        auto casts = findAllPossibleCasts(from, to->optionalTarget, false);
        for (auto& cast: casts) {
          cast.insert(cast.end() - 1, CastComponent(CastComponentType::Wrap));
        }
        results.insert(results.end(), casts.begin(), casts.end());
      }
    }

    if (to->isUnion() && to->indirectionLevel() == 0) {
      if (from->pointerLevel() == 0 && from->isUnion()) {
        throw std::runtime_error("multicast is not implemented");
      } else {
        for (auto& member: to->unionOf) {
          auto casts = findAllPossibleCasts(from, member, false);
          for (auto& cast: casts) {
            cast.insert(cast.end() - 1, CastComponent(CastComponentType::Widen, to, member));
          }
          results.insert(results.end(), casts.begin(), casts.end());
        }
      }
    }
  }

  if (from->pointerLevel() == 0 && from->isUnion()) {
    if (manual && !to->isUnion()) {
      for (auto& member: from->unionOf) {
        auto casts = findAllPossibleCasts(member, to, false);
        for (auto& cast: casts) {
          cast.insert(cast.begin(), CastComponent(CastComponentType::Narrow, member));
        }
        results.insert(results.end(), casts.begin(), casts.end());
      }
    }
  }

  auto noConstFrom = from->deconstify();
  auto noConstTo = to->deconstify();

  if (!from->isAny && !(*from == *noConstFrom)) {
    auto casts = findAllPossibleCasts(noConstFrom, to, manual);
    results.insert(results.end(), casts.begin(), casts.end());
  }

  if (!to->isAny && !(*to == *noConstTo)) {
    auto casts = findAllPossibleCasts(from, noConstTo, manual);
    results.insert(results.end(), casts.begin(), casts.end());
  }

  return results;
};

auto AltaCore::DET::Type::findCast(std::shared_ptr<Type> from, std::shared_ptr<Type> to, bool manual) -> CastPath {
  using CC = CastComponent;
  using CCT = CastComponentType;
  using SCT = SpecialCastType;
  using NT = NativeType;

  std::function<CastPath(std::vector<std::shared_ptr<Type>>, std::shared_ptr<Type>, size_t*)> reverseLoop = nullptr;
  std::function<CastPath(std::shared_ptr<Type>, std::vector<std::shared_ptr<Type>>, size_t*)> loop = nullptr;

  bool dontReverseLoopBack = false;

  reverseLoop = [&loop, &reverseLoop, &manual, &dontReverseLoopBack](std::vector<std::shared_ptr<Type>> froms, std::shared_ptr<Type> to, size_t* index) -> CastPath {
    #define AC_FROM_LOOP for (size_t i = 0; i < froms.size(); ++i) { auto& from = froms[i];
    #define AC_FROM_LOOP_END }
    #define AC_RETURN_INDEX if (index) { *index = i; }

    if (to->isAny) {
      return { CC(CCT::Destination) };
    }

    AC_FROM_LOOP;
      if (*from == *to) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination) };
      }
    AC_FROM_LOOP_END;

    decltype(froms) deconstFroms;
    bool deconstSame = true;
    AC_FROM_LOOP;
      auto deconst = from->deconstify();
      deconstFroms.push_back(deconst);
      if (!deconst->isAny && !(*deconst == *from)) {
        deconstSame = false;
      }
    AC_FROM_LOOP_END;

    if (!deconstSame) {
      auto cast = reverseLoop(deconstFroms, to, index);
      if (cast.size() > 0) {
        return cast;
      }
    };

    auto deconstTo = to->deconstify();
    auto noconstTo = to->deconstify(true);
    if (!to->isAny && !(*to == *deconstTo)) {
      auto cast = reverseLoop(froms, deconstTo, index);
      if (cast.size() > 0) {
        return cast;
      }
    }
    if (!to->isAny && !(*to == *noconstTo)) {
      auto cast = reverseLoop(froms, noconstTo, index);
      if (cast.size() > 0) {
        return cast;
      }
    }

    AC_FROM_LOOP;
      if (from->indirectionLevel() > 0 && to->indirectionLevel() > 0 && from->klass && to->klass && to->klass->hasParent(from->klass)) {
        AC_RETURN_INDEX;
        return { CC(CCT::Downcast, to->klass), CC(CCT::Destination) };
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (
        (
          (from->referenceLevel() <= 1 && to->referenceLevel() <= 1) ||
          (from->pointerLevel() <= 1 && to->pointerLevel() == from->pointerLevel())
        ) &&
        from->klass &&
        to->klass &&
        from->klass->hasParent(to->klass)
      ) {
        auto accessorClass = from->klass;
        CastPath cast;
        size_t i = 0;
        while (i < accessorClass->parents.size()) {
          auto& parent = accessorClass->parents[i];
          if (parent->id == to->klass->id) {
            cast.push_back(CC(CCT::Upcast, parent));
            break;
          }
          if (parent->hasParent(to->klass)) {
            cast.push_back(CC(CCT::Upcast, parent));
            i = 0;
            accessorClass = parent;
            continue;
          }
          ++i;
        }
        AC_RETURN_INDEX;
        cast.push_back(CC(CCT::Destination));
        return cast;
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isOptional && to->nativeTypeName == NativeType::Bool) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::OptionalPresent) };
      }

      if (to->indirectionLevel() == 0 && to->isOptional && from->pointerLevel() == 1 && from->isAny) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::EmptyOptional) };
      }

      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isFunction && to->isFunction && from->isRawFunction && !to->isRawFunction) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::WrapFunction) };
      }
    AC_FROM_LOOP_END;

    std::function<CastPath(std::shared_ptr<Type>, std::shared_ptr<Type>)> doFromOrToLoop = nullptr;
    doFromOrToLoop = [&](std::shared_ptr<Type> from, std::shared_ptr<Type> to) -> CastPath {
      if (from->indirectionLevel() == 0 && from->klass) {
        for (auto& method: from->klass->toCasts) {
          auto& special = method->returnType;
          if (*special == *to || *special == *to->deconstify() || *special == *to->deconstify(true)) {
            return { CC(CCT::To, method), CC(CCT::Destination) };
          }
          auto cast = doFromOrToLoop(special, to);
          if (cast.size() > 0) {
            cast.insert(cast.begin(), CC(CCT::To, method));
            return cast;
          }
        }
      }
      if (to->indirectionLevel() == 0 && to->klass) {
        for (auto& method: to->klass->fromCasts) {
          auto& special = method->parameterVariables.front()->type;
          if (*from == *special || *from == *special->deconstify() || *from == *special->deconstify(true)) {
            return { CC(CCT::From, method), CC(CCT::Destination) };
          }
          auto cast = doFromOrToLoop(from, special);
          if (cast.size() > 0) {
            cast.insert(cast.end() - 1, CC(CCT::From, method));
          }
        }
      }
      return {};
    };
    AC_FROM_LOOP;
      auto cast = doFromOrToLoop(from, to);
      if (cast.size() > 0) {
        AC_RETURN_INDEX;
        return cast;
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (!from->isOptional && to->isOptional && to->indirectionLevel() == 0) {
        auto cast = findCast(from, to->optionalTarget, false);
        if (cast.size() > 0) {
          cast.insert(cast.end() - 1, CC(CCT::Wrap));
        }
        AC_RETURN_INDEX;
        return cast;
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (!from->isUnion() && to->isUnion() && to->indirectionLevel() == 0) {
        size_t idx = 0;
        auto cast = loop(from, to->unionOf, &idx);
        if (cast.size() > 0) {
          cast.insert(cast.end() - 1, CC(CCT::Widen, to, to->unionOf[idx]));
        }
        AC_RETURN_INDEX;
        return cast;
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (
        from->indirectionLevel() == 0 &&
        to->indirectionLevel() == 0 &&
        from->isNative &&
        to->isNative &&
        (
          from->nativeTypeName == NT::Float ||
          from->nativeTypeName == NT::Double
        ) &&
        (
          to->nativeTypeName == NT::Float ||
          to->nativeTypeName == NT::Double
        )
      ) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isNative && to->isNative && !from->isAny && !to->isAny) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_FROM_LOOP_END;

    if (manual) {
      AC_FROM_LOOP;
        if (from->referenceLevel() == 0 && to->referenceLevel() == 0 && from->pointerLevel() > 0 && to->pointerLevel() > 0) {
          AC_RETURN_INDEX;
          return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
        }
      AC_FROM_LOOP_END;
    }

    AC_FROM_LOOP;
      if (
        from->referenceLevel() == 0 &&
        to->referenceLevel() == 0 &&
        from->pointerLevel() > 0 &&
        to->pointerLevel() == 0 &&
        to->isNative &&
        (
          to->nativeTypeName == NT::Integer ||
          to->nativeTypeName == NT::Byte
        )
      ) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (from->referenceLevel() == 0 && from->pointerLevel() > 0 && to->indirectionLevel() == 0 && to->isNative && to->nativeTypeName == NT::Bool) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::TestPointer) };
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (!dontReverseLoopBack && from->referenceLevel() > 0) {
        dontReverseLoopBack = true;
        auto other = from->dereference();
        while (other->referenceLevel() >= 0) {
          auto cast = loop(other, { to }, index);
          if (cast.size() > 0) {
            for (size_t i = other->referenceLevel(); i < from->referenceLevel(); ++i) {
              cast.insert(cast.begin(), CC(CCT::Dereference));
            }
            AC_RETURN_INDEX;
            dontReverseLoopBack = false;
            return cast;
          }
          if (other->referenceLevel() == 0) break;
        }
      }
    AC_FROM_LOOP_END;

    AC_FROM_LOOP;
      if (!dontReverseLoopBack && from->referenceLevel() < to->referenceLevel()) {
        dontReverseLoopBack = true;
        auto cast = loop(from->reference(), { to }, index);
        if (cast.size() > 0) {
          cast.insert(cast.begin(), CC(CCT::Reference));
          AC_RETURN_INDEX;
          dontReverseLoopBack = false;
          return cast;
        }
      };
    AC_FROM_LOOP_END;

    dontReverseLoopBack = false;

    if (index) *index = SIZE_MAX;
    return {};

    #undef AC_FROM_LOOP
    #undef AC_FROM_LOOP_END
    #undef AC_RETURN_INDEX
  };

  bool dontLoopBack = false;

  loop = [&loop, &reverseLoop, &manual, &dontLoopBack](std::shared_ptr<Type> from, std::vector<std::shared_ptr<Type>> tos, size_t* index) -> CastPath {
    if (manual && from->isUnion() && tos.size() == 1 && !tos.front()->isUnion() && from->indirectionLevel() == 0) {
      size_t idx = 0;
      auto cast = reverseLoop(from->unionOf, tos.front(), &idx);
      if (cast.size() > 0) {
        cast.insert(cast.begin(), CC(CCT::Narrow, from->unionOf[idx]));
        if (index) *index = 0;
        return cast;
      }
      if (index) *index = SIZE_MAX;
      return {};
    }

    #define AC_TO_LOOP for (size_t i = 0; i < tos.size(); ++i) { auto& to = tos[i];
    #define AC_TO_LOOP_END }
    #define AC_RETURN_INDEX if (index) { *index = i; }

    auto saveManual = [&]() {
      auto tmp = manual;
      manual = false;
      return tmp;
    };
    auto restoreManual = [&](bool man) {
      manual = man;
    };

    AC_TO_LOOP;
      if (*from == *to) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    if (from->isAny && from->pointerLevel() == 1) {
      AC_TO_LOOP;
        if (to->indirectionLevel() == 0 && to->isOptional && from->pointerLevel() == 1 && from->isAny) {
          AC_RETURN_INDEX;
          return { CC(CCT::Destination, SCT::EmptyOptional) };
        }
      AC_TO_LOOP_END;
      return { CC(CCT::Destination) };
    }

    decltype(tos) deconstTos;
    decltype(tos) noconstTos;
    bool deconstSame = true;
    bool noconstSame = true;
    AC_TO_LOOP;
      auto deconst = to->deconstify();
      auto noconst = to->deconstify(true);
      deconstTos.push_back(deconst);
      noconstTos.push_back(noconst);
      if (!deconst->isAny && !(*deconst == *to)) {
        deconstSame = false;
      }
      if (!noconst->isAny && !(*noconst == *to)) {
        noconstSame = false;
      }
    AC_TO_LOOP_END;

    if (!deconstSame) {
      auto cast = loop(from, deconstTos, index);
      if (cast.size() > 0) {
        return cast;
      }
    };

    if (!noconstSame) {
      auto cast = loop(from, noconstTos, index);
      if (cast.size() > 0) {
        return cast;
      }
    };

    auto deconstFrom = from->deconstify();
    if (!deconstFrom->isAny && !(*from == *deconstFrom)) {
      auto cast = loop(deconstFrom, tos, index);
      if (cast.size() > 0) {
        return cast;
      }
    }

    AC_TO_LOOP;
      if (from->indirectionLevel() > 0 && to->indirectionLevel() > 0 && from->klass && to->klass && to->klass->hasParent(from->klass)) {
        AC_RETURN_INDEX;
        return { CC(CCT::Downcast, to->klass), CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (
        (
          (from->referenceLevel() <= 1 && to->referenceLevel() <= 1) ||
          (from->pointerLevel() <= 1 && to->pointerLevel() == from->pointerLevel())
        ) &&
        from->klass &&
        to->klass &&
        from->klass->hasParent(to->klass)
      ) {
        auto accessorClass = from->klass;
        CastPath cast;
        if (from->referenceLevel() > 0) {
          cast.push_back(CC(CCT::Dereference));
        }
        size_t i = 0;
        while (i < accessorClass->parents.size()) {
          auto& parent = accessorClass->parents[i];
          if (parent->id == to->klass->id) {
            cast.push_back(CC(CCT::Upcast, parent));
            break;
          }
          if (parent->hasParent(to->klass)) {
            cast.push_back(CC(CCT::Upcast, parent));
            i = 0;
            accessorClass = parent;
            continue;
          }
          ++i;
        }
        if (to->referenceLevel() > 0) {
          cast.push_back(CC(CCT::Reference));
        }
        AC_RETURN_INDEX;
        cast.push_back(CC(CCT::Destination));
        return cast;
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isOptional && to->nativeTypeName == NativeType::Bool) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::OptionalPresent) };
      }

      if (to->indirectionLevel() == 0 && to->isOptional && from->pointerLevel() == 1 && from->isAny) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::EmptyOptional) };
      }

      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isFunction && to->isFunction && from->isRawFunction && !to->isRawFunction) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::WrapFunction) };
      }
    AC_TO_LOOP_END;

    std::function<CastPath(std::shared_ptr<Type>, std::shared_ptr<Type>)> doFromOrToLoop = nullptr;
    doFromOrToLoop = [&](std::shared_ptr<Type> from, std::shared_ptr<Type> to) -> CastPath {
      if (from->indirectionLevel() == 0 && from->klass) {
        for (auto& method: from->klass->toCasts) {
          auto& special = method->returnType;
          if (*special == *to || *special == *to->deconstify() || *special == *to->deconstify(true)) {
            return { CC(CCT::To, method), CC(CCT::Destination) };
          }
          auto cast = doFromOrToLoop(special, to);
          if (cast.size() > 0) {
            cast.insert(cast.begin(), CC(CCT::To, method));
            return cast;
          }
        }
      }
      if (to->indirectionLevel() == 0 && to->klass) {
        for (auto& method: to->klass->fromCasts) {
          auto& special = method->parameterVariables.front()->type;
          if (*from == *special || *from == *special->deconstify() || *from == *special->deconstify(true)) {
            return { CC(CCT::From, method), CC(CCT::Destination) };
          }
          auto cast = doFromOrToLoop(from, special);
          if (cast.size() > 0) {
            cast.insert(cast.end() - 1, CC(CCT::From, method));
          }
        }
      }
      return {};
    };
    AC_TO_LOOP;
      auto cast = doFromOrToLoop(from, to);
      if (cast.size() > 0) {
        AC_RETURN_INDEX;
        return cast;
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (!from->isOptional && to->isOptional && to->indirectionLevel() == 0) {
        auto cast = findCast(from, to->optionalTarget, false);
        if (cast.size() > 0) {
          cast.insert(cast.end() - 1, CC(CCT::Wrap));
        }
        AC_RETURN_INDEX;
        return cast;
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (!from->isUnion() && to->isUnion() && to->indirectionLevel() == 0) {
        size_t idx = 0;
        auto manu = saveManual();
        auto cast = loop(from, to->unionOf, &idx);
        restoreManual(manu);
        if (cast.size() > 0) {
          cast.insert(cast.end() - 1, CC(CCT::Widen, to, to->unionOf[idx]));
        }
        AC_RETURN_INDEX;
        return cast;
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (from->isUnion() && from->pointerLevel() == 0 && to->isUnion() && to->indirectionLevel() == 0) {
        std::vector<std::pair<size_t, CastPath>> multicast;
        for (size_t j = 0; j < from->unionOf.size(); ++j) {
          size_t idx = 0;
          auto manu = saveManual();
          auto cast = loop(from->unionOf[j], to->unionOf, &idx);
          restoreManual(manu);
          if (cast.size() == 0) {
            idx = SIZE_MAX;
          }
          multicast.emplace_back(idx, cast);
        }
        auto tmp = CC(CCT::Multicast, multicast);
        tmp.target = to;
        return { tmp, CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (
        from->indirectionLevel() == 0 &&
        to->indirectionLevel() == 0 &&
        from->isNative &&
        to->isNative &&
        (
          from->nativeTypeName == NT::Float ||
          from->nativeTypeName == NT::Double
        ) &&
        (
          to->nativeTypeName == NT::Float ||
          to->nativeTypeName == NT::Double
        )
      ) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isNative && to->isNative && !from->isAny && !to->isAny) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    if (manual) {
      AC_TO_LOOP;
        if (from->referenceLevel() == 0 && to->referenceLevel() == 0 && from->pointerLevel() > 0 && to->pointerLevel() > 0) {
          AC_RETURN_INDEX;
          return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
        }
      AC_TO_LOOP_END;
    }

    AC_TO_LOOP;
      if (
        from->referenceLevel() == 0 &&
        to->referenceLevel() == 0 &&
        from->pointerLevel() > 0 &&
        to->pointerLevel() == 0 &&
        to->isNative &&
        (
          to->nativeTypeName == NT::Integer ||
          to->nativeTypeName == NT::Byte
        )
      ) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (
        from->referenceLevel() == 0 &&
        to->referenceLevel() == 0 &&
        from->pointerLevel() == 0 &&
        to->pointerLevel() > 0 &&
        from->isNative &&
        (
          from->nativeTypeName == NT::Integer ||
          from->nativeTypeName == NT::Byte
        )
      ) {
        AC_RETURN_INDEX;
        return { CC(CCT::SimpleCoercion, to), CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    AC_TO_LOOP;
      if (from->referenceLevel() == 0 && from->pointerLevel() > 0 && to->indirectionLevel() == 0 && to->isNative && to->nativeTypeName == NT::Bool) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination, SCT::TestPointer) };
      }
    AC_TO_LOOP_END;

    if (!dontLoopBack && from->referenceLevel() > 0) {
      dontLoopBack = true;
      auto other = from->dereference();
      while (other->referenceLevel() >= 0) {
        auto cast = loop(other, tos, index);
        if (cast.size() > 0) {
          for (size_t i = other->referenceLevel(); i < from->referenceLevel(); ++i) {
            cast.insert(cast.begin(), CC(CCT::Dereference));
          }
          return cast;
        }
        if (other->referenceLevel() == 0) break;
      }
    }

    size_t maxToRefLevel = 0;
    AC_TO_LOOP;
      if (to->referenceLevel() > maxToRefLevel) {
        maxToRefLevel = to->referenceLevel();
      }
    AC_TO_LOOP_END;

    if (!dontLoopBack && from->referenceLevel() < maxToRefLevel) {
      dontLoopBack = true;
      auto cast = loop(from->reference(), tos, index);
      if (cast.size() > 0) {
        cast.insert(cast.begin(), CC(CCT::Reference));
        dontLoopBack = false;
        return cast;
      }
    };

    dontLoopBack = false;

    AC_TO_LOOP;
      if (to->isAny) {
        AC_RETURN_INDEX;
        return { CC(CCT::Destination) };
      }
    AC_TO_LOOP_END;

    if (manual) {
      return { CC(CCT::SimpleCoercion, tos.front()), CC(CCT::Destination) };
    }

    if (index) *index = SIZE_MAX;
    return {};

    #undef AC_TO_LOOP
    #undef AC_TO_LOOP_END
    #undef AC_RETURN_INDEX
  };
  return loop(from, { to }, nullptr);
};

size_t AltaCore::DET::Type::compatiblity(const AltaCore::DET::Type& other) const {
  if (other.isAccessor) return compatiblity(*other.returnType);
  size_t compat = SIZE_MAX - 5;

  if (isAny || other.isAny) return 1;

  if (isExactlyCompatibleWith(other)) return SIZE_MAX;
  if (isExactlyCompatibleWith(*other.deconstify())) return SIZE_MAX - 1;
  if (deconstify()->isExactlyCompatibleWith(other)) return SIZE_MAX - 2;

  auto nonConstPtrTo = other.deconstify();
  for (size_t i = 0; i < nonConstPtrTo->modifiers.size(); ++i) {
    auto& mod = nonConstPtrTo->modifiers[i];
    if (mod & (uint8_t)AltaCore::DET::TypeModifierFlag::Constant) {
      mod &= ~((uint8_t)AltaCore::DET::TypeModifierFlag::Constant);
    }
    if (mod == 0) {
      nonConstPtrTo->modifiers.erase(nonConstPtrTo->modifiers.begin() + i);
      --i;
    }
  }
  if (isExactlyCompatibleWith(*nonConstPtrTo)) return SIZE_MAX - 3;

  auto nonConstPtrFrom = deconstify();
  for (size_t i = 0; i < nonConstPtrFrom->modifiers.size(); ++i) {
    auto& mod = nonConstPtrFrom->modifiers[i];
    if (mod & (uint8_t)AltaCore::DET::TypeModifierFlag::Constant) {
      mod &= ~((uint8_t)AltaCore::DET::TypeModifierFlag::Constant);
    }
    if (mod == 0) {
      nonConstPtrFrom->modifiers.erase(nonConstPtrFrom->modifiers.begin() + i);
      --i;
    }
  }
  if (nonConstPtrFrom->isExactlyCompatibleWith(other)) return SIZE_MAX - 4;

  if (other.pointerLevel() == 0 && other.klass) {
    if (auto to = other.klass->findToCast(*this)) {
      return 2;
    }
  }
  if (pointerLevel() == 0 && klass) {
    if (auto from = klass->findFromCast(other)) {
      return 2;
    }
  }

  if (!commonCompatiblity(other)) return 0;

  if (other.isNative && other.pointerLevel() < 1 && pointerLevel() > 0) {
    compat -= 5;
  }

  if (isOptional) {
    if (!other.isOptional && pointerLevel() > 0) return 0;
    if (other.isOptional && pointerLevel() != other.pointerLevel()) return 0;
    return optionalTarget->compatiblity(other.isOptional ? *other.optionalTarget : other);
  }

  if (referenceLevel() < other.referenceLevel()) {
    compat -= 2;
  } else if (referenceLevel() > other.referenceLevel()) {
    compat -= 1;
  }

  if (modifiers.size() == other.modifiers.size()) {
    bool modsEqual = true;
    for (size_t i = 0; i < modifiers.size(); i++) {
      if (modifiers[i] != other.modifiers[i]) {
        modsEqual = false;
        break;
      }
    }
    if (!modsEqual) {
      compat -= 1;
    }
  }

  if (isFunction) {
    auto retCompat = returnType->compatiblity(*other.returnType);
    if (retCompat == 0) return 0;
    compat -= SIZE_MAX - retCompat;
    if (parameters.size() != other.parameters.size()) return 0;
    for (size_t i = 0; i < parameters.size(); i++) {
      auto paramCompat = std::get<1>(parameters[i])->compatiblity(*std::get<1>(other.parameters[i]));
      if (paramCompat == 0) return 0;
      if (std::get<0>(parameters[0]) == std::get<0>(other.parameters[0])) ++paramCompat;
      compat -= SIZE_MAX - paramCompat;
    }
  } else if (isNative) {
    if ((nativeTypeName == NativeType::Void) != (other.nativeTypeName == NativeType::Void)) {
      return 0;
    }
    if (!(nativeTypeName == other.nativeTypeName && (nativeTypeName != NativeType::UserDefined || userDefinedName == other.userDefinedName))) {
      compat -= 1;
    }
  } else if (isUnion()) {
    if (unionOf.size() != other.unionOf.size()) compat -= 1;
    if (other.unionOf.size() > 0) {
      for (auto& otherItem: other.unionOf) {
        size_t greatestCompat = 0;
        for (auto& item: unionOf) {
          auto curr = item->compatiblity(*otherItem);
          if (curr > greatestCompat) {
            greatestCompat = curr;
          }
        }
        if (greatestCompat == 0) {
          return 0;
        }
        compat -= SIZE_MAX - greatestCompat;
      }
    } else {
      size_t greatestCompat = 0;
      for (auto& item: unionOf) {
        auto curr = item->compatiblity(other);
        if (curr > greatestCompat) {
          greatestCompat = curr;
        }
      }
      if (greatestCompat == 0) {
        return 0;
      }
      compat -= SIZE_MAX - greatestCompat;
    }
  } else {
    if (klass->id != other.klass->id) {
      compat -= 1;
    }
  }

  return compat;
};

bool AltaCore::DET::Type::commonCompatiblity(const AltaCore::DET::Type& other, bool strict) const {
  if (referenceLevel() > 0) return destroyReferences()->commonCompatiblity(other, strict);
  if (other.referenceLevel() > 0) return commonCompatiblity(*other.destroyReferences(), strict);
  if (other.isAccessor) return commonCompatiblity(*other.returnType, strict);

  if (!strict) {
    if (other.pointerLevel() == 0 && other.klass && other.klass->findToCast(*this)) return true;
    if (pointerLevel() == 0 && klass && klass->findFromCast(*this)) return true;
  }

  if (isOptional && other.isAny && other.pointerLevel() == 1) return true;
  if (isAny || other.isAny) {
    // little hack to only assign `nullptr` to pointers
    if (
      (isAny && pointerLevel() == 1 && other.pointerLevel() == 0) ||
      (other.isAny && other.pointerLevel() == 1 && pointerLevel() == 0)
    ) return false;
    return true;
  }
  if (!isOptional && other.isOptional) return false;
  if (isOptional) {
    if (!other.isOptional && pointerLevel() > 0) return false;
    if (other.isOptional && pointerLevel() != other.pointerLevel()) return false;
    return optionalTarget->commonCompatiblity(other.isOptional ? *other.optionalTarget : other, strict);
  }
  if (!isUnion() && !other.isUnion()) {
    if (other.isNative && other.pointerLevel() < 1 && pointerLevel() > 0) return true;
    if (isFunction != other.isFunction) return false;
    if (isNative != other.isNative) return false;
    if (!isNative && klass->id != other.klass->id && !other.klass->hasParent(klass)) return false;
    if (pointerLevel() != other.pointerLevel()) return false;
  }

  // we can widen from the source (e.g. `other`) to the destination (e.g. `this`),
  // but not the other way around
  if (unionOf.size() < other.unionOf.size()) return false;
  if (other.unionOf.size() > 0) {
    for (auto& otherItem: other.unionOf) {
      bool found = false;
      for (auto& item: unionOf) {
        if (item->commonCompatiblity(*otherItem, strict)) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    }
  } else if (isUnion()) {
    bool found = false;
    for (auto& item: unionOf) {
      if (item->commonCompatiblity(other, strict)) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }

  /**
   * here's the reasoning behind this check:
   * 
   * the current type is the destination type (e.g. the type of a function parameter)
   * the `other` type is the source type (e.g. the type of an argument in a function call)
   * 
   * we can automatically ref the source type *once* if needed, but no more than once
   * (because you can't get the address of an address)
   */
  if (referenceLevel() > other.referenceLevel() + 1) return false;
  /**
   * why don't we also check the reference level the other way around?
   * because we can deref the source type as many times as we need
   * (because in C, for example, you can just keep following pointers as many times as you need to)
   */

  if (isFunction) {
    if (parameters.size() != other.parameters.size()) return false;
    if (isRawFunction && !other.isRawFunction) return false;
  }

  return true;
};

bool AltaCore::DET::Type::isExactlyCompatibleWith(const AltaCore::DET::Type& other) const {
  if (other.isAccessor) return isExactlyCompatibleWith(*other.returnType);
  if (!commonCompatiblity(other, true)) return false;
  if (isAny || other.isAny) return false;
  if (isOptional != other.isOptional) return false;
  if (unionOf.size() != other.unionOf.size()) return false;

  if (isUnion()) {
    for (auto& otherItem: other.unionOf) {
      bool found = false;
      for (auto& item: unionOf) {
        if (item->isExactlyCompatibleWith(*otherItem)) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    }
  }

  // here, we care about *exact* compatability, and that includes all modifiers
  if (modifiers.size() != other.modifiers.size()) return false;
  for (size_t i = 0; i < modifiers.size(); i++) {
    if (modifiers[i] != other.modifiers[i]) return false;
  }

  if (isFunction) {
    if (!returnType->isExactlyCompatibleWith(*other.returnType)) return false;
    for (size_t i = 0; i < parameters.size(); i++) {
      if (!std::get<1>(parameters[i])->isExactlyCompatibleWith(*std::get<1>(other.parameters[i]))) return false;
    }
    if (isRawFunction != other.isRawFunction) return false;
  } else if (isNative) {
    if (nativeTypeName != other.nativeTypeName) return false;
    if (userDefinedName != other.userDefinedName) return false;
  } else if (!isUnion() && !isOptional) {
    if (klass->id != other.klass->id) return false;
  }

  return true;
};

bool AltaCore::DET::Type::isCompatibleWith(const AltaCore::DET::Type& other) const {
  if (other.isAccessor) return isCompatibleWith(*other.returnType);
  if (other.referenceLevel() > 0) return isCompatibleWith(*other.destroyReferences());
  if (referenceLevel() > 0) return destroyReferences()->isCompatibleWith(other);

  if (isExactlyCompatibleWith(other)) return true;

  if (other.pointerLevel() == 0 && other.klass && other.klass->findToCast(*this)) return true;
  if (pointerLevel() == 0 && klass && klass->findFromCast(*this)) return true;

  // integers can be converted to pointers
  if (other.isNative && other.pointerLevel() < 1 && pointerLevel() > 0) return true;

  if (!commonCompatiblity(other)) return false;
  if (isOptional && other.isAny && other.pointerLevel() == 1) return true;
  if (isOptional) return optionalTarget->isCompatibleWith(other.isOptional ? *other.optionalTarget : other);
  if (isAny || other.isAny) return true;

  if (other.unionOf.size() > 0) {
    for (auto& otherItem: other.unionOf) {
      bool found = false;
      for (auto& item: unionOf) {
        if (item->isCompatibleWith(*otherItem)) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    }
  } else if (isUnion()) {
    bool found = false;
    for (auto& item: unionOf) {
      if (item->isCompatibleWith(other)) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }

  if (isFunction) {
    if (!returnType->isCompatibleWith(*other.returnType)) return false;
    if (parameters.size() != other.parameters.size()) return false;
    for (size_t i = 0; i < parameters.size(); i++) {
      if (!std::get<1>(parameters[i])->isCompatibleWith(*std::get<1>(other.parameters[i]))) return false;
    }
  } else if (isNative) {
    // only check for void
    // all other native types are integral and can be coerced to each other
    if ((nativeTypeName == NativeType::Void) != (other.nativeTypeName == NativeType::Void)) {
      return false;
    }
  }

  return true;
};

AltaCore::DET::Type::Type(AltaCore::DET::NativeType _nativeTypeName, std::vector<uint8_t> _modifiers, std::string _userDefinedName):
  ScopeItem(""),
  isNative(true),
  isFunction(false),
  nativeTypeName(_nativeTypeName),
  modifiers(_modifiers),
  userDefinedName(_userDefinedName)
  {};
AltaCore::DET::Type::Type(std::shared_ptr<AltaCore::DET::Type> _returnType, std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> _parameters, std::vector<uint8_t> _modifiers, bool _isRawFunction):
  ScopeItem(""),
  isNative(true),
  isFunction(true),
  returnType(_returnType),
  parameters(_parameters),
  modifiers(_modifiers),
  isRawFunction(_isRawFunction)
  {};
AltaCore::DET::Type::Type(std::shared_ptr<AltaCore::DET::Class> _klass, std::vector<uint8_t> _modifiers):
  ScopeItem(""),
  isNative(false),
  isFunction(false),
  klass(_klass),
  modifiers(_modifiers)
  {};
AltaCore::DET::Type::Type(std::vector<std::shared_ptr<AltaCore::DET::Type>> _unionOf, std::vector<uint8_t> _modifiers):
  ScopeItem(""),
  isNative(false),
  isFunction(false),
  modifiers(_modifiers)
{
  // loop through the _unionOf types and unpack any union types
  std::stack<std::pair<decltype(_unionOf), size_t>> indexes;
  indexes.push(std::make_pair(_unionOf, 0));
  while (indexes.size() > 0) {
    auto& [uni, idx] = indexes.top();
    if (idx >= uni.size()) {
      indexes.pop();
      continue;
    }
    auto& type = uni[idx++];
    if (type->unionOf.size() > 0) {
      indexes.push(std::make_pair(type->unionOf, 0));
    } else {
      unionOf.push_back(type);
    }
  }
};

bool AltaCore::DET::Type::operator %(const AltaCore::DET::Type& other) const {
  return isCompatibleWith(other);
};

bool AltaCore::DET::Type::operator ==(const AltaCore::DET::Type& other) const {
  return isExactlyCompatibleWith(other);
};

const size_t AltaCore::DET::Type::requiredArgumentCount() const {
  size_t count = 0;
  for (auto [name, type, isVariable, id]: parameters) {
    if (!isVariable) {
      count++;
    }
  }
  return count;
};

bool AltaCore::DET::Type::includes(const std::shared_ptr<Type> otherType) const {
  for (auto& item: unionOf) {
    if (item->isExactlyCompatibleWith(*otherType)) {
      return true;
    }
  }
  return false;
};


auto AltaCore::DET::Type::determineCompatiblity(std::shared_ptr<Type> from, std::shared_ptr<Type> to, CastPath cast) -> CastCompatibility {
  using CCT = CastComponentType;

  CastCompatibility compat = MAX_CAST_COMPAT;

  /**
   * cast precedence scale:
   * 1. destination
   * 2. to
   * 3. from
   * 4. upcast
   * 5. wrap
   * 6. widen
   * 7. dereference
   * 8. multicast
   * 9. downcast
   * 10. narrow
   * 11. unwrap
   * 12. reference
   * 13. simple coercion
   *
   * this scale is multiplied by the current depth in order to guarantee that casts with less steps
   * are more important than casts with more steps
   * 
   * in addition, types are preferred in accordance with this scale:
   * 1. native
   * 2. object
   * 3. reference wrapped
   * 4. pointer wrapped
   * 5. optional
   * 6. union
   */

  auto levelType = from;

  for (size_t i = 0; i < cast.size(); i++) {
    auto& level = cast[i];
    CastCompatibility levelWeight = 0;
    CastCompatibility specialWeight = 0;

    #define AC_CCT_LEVEL(type, weight) case CCT::type: levelWeight = weight; break;
    switch (level.type) {
      AC_CCT_LEVEL(Destination, 0);
      AC_CCT_LEVEL(To, 1);
      AC_CCT_LEVEL(From, 2);
      AC_CCT_LEVEL(Upcast, 3);
      AC_CCT_LEVEL(Wrap, 4);
      AC_CCT_LEVEL(Widen, 5);
      AC_CCT_LEVEL(Dereference, 6);
      AC_CCT_LEVEL(Multicast, 7);
      AC_CCT_LEVEL(Downcast, 8);
      AC_CCT_LEVEL(Narrow, 9);
      AC_CCT_LEVEL(Unwrap, 10);
      AC_CCT_LEVEL(Reference, 11);
      AC_CCT_LEVEL(SimpleCoercion, 12);
      default:
        throw std::runtime_error("unknown cast type");
    }
    #undef AC_CCT_LEVEL

    compat -= levelWeight * (i + 1);

    if (level.type == CCT::SimpleCoercion) {
      levelType = level.target;
    } else if (level.type == CCT::To) {
      levelType = level.method->returnType;
    } else if (level.type == CCT::From) {
      levelType = std::make_shared<DET::Type>(level.method->parentScope.lock()->parentClass.lock());
    } else if (level.type == CCT::Upcast) {
      auto curr = levelType;
      levelType = std::make_shared<DET::Type>(level.klass);
      if (curr->pointerLevel() > 0) {
        levelType = levelType->point();
      } else if (curr->referenceLevel() > 0) {
        levelType = levelType->reference();
      }
    } else if (level.type == CCT::Wrap) {
      levelType = std::make_shared<DET::Type>(true, levelType);
    } else if (level.type == CCT::Widen) {
      levelType = level.target;
    } else if (level.type == CCT::Dereference) {
      levelType = levelType->dereference();
    } else if (level.type == CCT::Multicast) {
      throw std::runtime_error("multicast is unimplemented");
    } else if (level.type == CCT::Downcast) {
      auto curr = levelType;
      levelType = std::make_shared<DET::Type>(level.klass);
      if (curr->pointerLevel() > 0) {
        levelType = levelType->point();
      } else if (curr->referenceLevel() > 0) {
        levelType = levelType->reference();
      }
    } else if (level.type == CCT::Narrow) {
      levelType = level.target;
    } else if (level.type == CCT::Unwrap) {
      levelType = levelType->optionalTarget;
    } else if (level.type == CCT::Reference) {
      levelType = levelType->reference();
    }
  }

  return compat;
};

auto AltaCore::DET::Type::findMostCompatibleCast(std::shared_ptr<Type> from, std::shared_ptr<Type> to, std::vector<CastPath> casts) -> std::pair<CastPath, CastCompatibility> {
  std::pair<CastPath, CastCompatibility> mostCompatible = std::make_pair(CastPath(), 0);

  for (auto& cast: casts) {
    auto compat = determineCompatiblity(from, to, cast);

    // when casting a pointer type to a union,
    // bool-ification (i.e. casting via pointer testing)
    // should be the last option
    if (to->isUnion() && from->pointerLevel() > 0) {
      if (cast.back().special == SpecialCastType::TestPointer) {
        if (mostCompatible.second > 0) {
          compat = 0;
        }
      } else if (mostCompatible.first.size() == 0 || mostCompatible.first.back().special == SpecialCastType::TestPointer) {
        mostCompatible.second = 0;
      }
    }

    if (from->indirectionLevel() > 0 && to->indirectionLevel() == 1 && from->klass && to->klass && to->klass->hasParent(from->klass)) {
      if (cast.front() == CastComponentType::Downcast) {
        mostCompatible.second = 0;
      }
    }

    if (compat > mostCompatible.second) {
      mostCompatible = std::make_pair(cast, compat);
    }
  }

  return mostCompatible;
};
