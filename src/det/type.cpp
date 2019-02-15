#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast.hpp"
#include "../../include/altacore/util.hpp"

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
    return getUnderlyingType(assign->target.get());
  } else if (auto fetch = dynamic_cast<DH::Fetch*>(expression)) {
    if (!fetch->narrowedTo) {
      throw std::runtime_error("the given fetch has not been narrowed. either narrow it or use `AltaCore::DET::Type::getUnderlyingTypes` instead");
    }
    return getUnderlyingType(fetch->narrowedTo);
  } else if (auto boolean = dynamic_cast<DH::BooleanLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Bool, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto binOp = dynamic_cast<DH::BinaryOperation*>(expression)) {
    return getUnderlyingType(binOp->left.get());
  } else if (auto call = dynamic_cast<DH::FunctionCallExpression*>(expression)) {
    return call->targetType->returnType;
  } else if (auto acc = dynamic_cast<DH::Accessor*>(expression)) {
    if (!acc->narrowedTo) {
      throw std::runtime_error("the given accessor has not been narrowed. either narrow it or use `AltaCore::DET::Type::getUnderlyingTypes` instead");
    }
    return getUnderlyingType(acc->narrowedTo);
  } else if (auto str = dynamic_cast<DH::StringLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Byte, std::vector<uint8_t> { (uint8_t)Modifier::Constant | (uint8_t)Modifier::Pointer, (uint8_t)Modifier::Constant });
  } else if (auto cond = dynamic_cast<DH::ConditionalExpression*>(expression)) {
    return getUnderlyingType(cond->primaryResult.get());
  } else if (auto inst = dynamic_cast<DH::ClassInstantiationExpression*>(expression)) {
    return std::make_shared<Type>(inst->klass);
  } else if (auto ptr = dynamic_cast<DH::PointerExpression*>(expression)) {
    return getUnderlyingType(ptr->target.get())->point();
  } else if (auto deref = dynamic_cast<DH::DereferenceExpression*>(expression)) {
    return getUnderlyingType(deref->target.get())->follow();
  } else if (auto cast = dynamic_cast<DH::CastExpression*>(expression)) {
    return cast->type->type;
  } else if (auto chara = dynamic_cast<DH::CharacterLiteralNode*>(expression)) {
    return std::make_shared<Type>(NativeType::Byte, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (auto subs = dynamic_cast<DH::SubscriptExpression*>(expression)) {
    return getUnderlyingType(subs->target.get())->follow();
  }

  return nullptr;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::getUnderlyingType(std::shared_ptr<AltaCore::DET::ScopeItem> item) {
  using ItemType = DET::NodeType;
  using Modifier = AST::TypeModifierFlag;
  ItemType itemType = item->nodeType();

  if (itemType == ItemType::Function) {
    auto func = std::dynamic_pointer_cast<Function>(item);
    std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> params;
    for (auto& [name, type, isVariable, id]: func->parameters) {
      params.push_back(std::make_tuple(name, type, isVariable, id));
    }
    auto type = std::make_shared<Type>(func->returnType, params);
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
  } else {
    throw std::runtime_error("Only functions and variables have underlying types");
  }
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

std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::reference() {
  auto other = copy();
  other->modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Reference);
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::dereference() {
  auto other = copy();
  if (other->modifiers.size() > 0) {
    auto idx = other->modifiers.size() - 1;
    other->modifiers[idx] &= ~(uint8_t)Shared::TypeModifierFlag::Reference;
    // `pop_back` if the modifier level is now empty
    // why keep around a useless entry in the vector?
    if (other->modifiers[idx] == 0) {
      other->modifiers.pop_back();
    }
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::point() {
  auto other = copy();
  other->modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Pointer);
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::follow() {
  auto other = copy();
  if (other->modifiers.size() > 0) {
    auto idx = other->modifiers.size() - 1;
    other->modifiers[idx] &= ~(uint8_t)Shared::TypeModifierFlag::Pointer;
    if (other->modifiers[idx] == 0) {
      other->modifiers.pop_back();
    }
  }
  return other;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::followBlindly() {
  auto other = copy();
  if (other->modifiers.size() > 0) {
    auto idx = other->modifiers.size() - 1;
    other->modifiers[idx] &= ~((uint8_t)Shared::TypeModifierFlag::Reference | (uint8_t)Shared::TypeModifierFlag::Pointer);
    if (other->modifiers[idx] == 0) {
      other->modifiers.pop_back();
    }
  }
  return other;
};
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
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Pointer) {
      count++;
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
  }

  return count;
};

size_t AltaCore::DET::Type::compatiblity(const AltaCore::DET::Type& other) {
  size_t compat = 2;

  if (isAny || other.isAny) return 1;

  if (isExactlyCompatibleWith(other)) return SIZE_MAX;
  if (!commonCompatiblity(other)) return 0;

  if (referenceLevel() == other.referenceLevel()) {
    compat++;
  }

  if (modifiers.size() == other.modifiers.size()) {
    bool modsEqual = true;
    for (size_t i = 0; i < modifiers.size(); i++) {
      if (modifiers[i] != other.modifiers[i]) {
        modsEqual = false;
        break;
      }
    }
    if (modsEqual) {
      compat++;
    }
  }

  if (isFunction) {
    auto retCompat = returnType->compatiblity(*other.returnType);
    if (retCompat == 0) return 0;
    compat += retCompat;
    if (parameters.size() != other.parameters.size()) return 0;
    for (size_t i = 0; i < parameters.size(); i++) {
      auto paramCompat = std::get<1>(parameters[i])->compatiblity(*std::get<1>(other.parameters[i]));
      if (paramCompat == 0) return 0;
      compat += paramCompat;
    }
  } else {
    if ((nativeTypeName == NativeType::Void) != (other.nativeTypeName == NativeType::Void)) {
      return 0;
    }
    if (nativeTypeName == other.nativeTypeName) {
      compat++;
    }
  }

  return compat;
};

bool AltaCore::DET::Type::commonCompatiblity(const AltaCore::DET::Type& other) {
  if (isAny || other.isAny) return true;
  if (isFunction != other.isFunction) return false;
  if (isNative != other.isNative) return false;
  if (!isNative && klass->id != other.klass->id) return false;
  if (pointerLevel() != other.pointerLevel()) return false;

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

  if (isFunction && parameters.size() != other.parameters.size()) return false;

  return true;
};

bool AltaCore::DET::Type::isExactlyCompatibleWith(const AltaCore::DET::Type& other) {
  if (!commonCompatiblity(other)) return false;
  if (isAny || other.isAny) return false;

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
  } else if (isNative) {
    if (nativeTypeName != other.nativeTypeName) return false;
  }

  return true;
};

bool AltaCore::DET::Type::isCompatibleWith(const AltaCore::DET::Type& other) {
  if (!commonCompatiblity(other)) return false;
  if (isAny || other.isAny) return true;

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

AltaCore::DET::Type::Type(AltaCore::DET::NativeType _nativeTypeName, std::vector<uint8_t> _modifiers):
  ScopeItem(""),
  isNative(true),
  isFunction(false),
  nativeTypeName(_nativeTypeName),
  modifiers(_modifiers)
  {};
AltaCore::DET::Type::Type(std::shared_ptr<AltaCore::DET::Type> _returnType, std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> _parameters, std::vector<uint8_t> _modifiers):
  ScopeItem(""),
  isNative(true),
  isFunction(true),
  returnType(_returnType),
  parameters(_parameters),
  modifiers(_modifiers)
  {};
AltaCore::DET::Type::Type(std::shared_ptr<AltaCore::DET::Class> _klass, std::vector<uint8_t> _modifiers):
  ScopeItem(""),
  isNative(false),
  isFunction(false),
  klass(_klass),
  modifiers(_modifiers)
  {};

bool AltaCore::DET::Type::operator %(const AltaCore::DET::Type& other) {
  return isCompatibleWith(other);
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
