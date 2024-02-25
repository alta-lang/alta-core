#include "../../include/altacore/det/class.hpp"
#include "../../include/altacore/det/variable.hpp"
#include "../../include/altacore/ast/class-definition-node.hpp"
#include "../../include/altacore/det/scope.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Class::nodeType() {
  return NodeType::Class;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Class::clone() {
  return std::make_shared<Class>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Class::deepClone() {
  auto self = std::dynamic_pointer_cast<Class>(clone());
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());

  return self;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::DET::Class::create(std::string name, std::shared_ptr<AltaCore::DET::Scope> parentScope, AltaCore::Errors::Position position, std::vector<std::shared_ptr<Class>> parents, bool isStructure) {
  auto klass = std::make_shared<Class>(name, parentScope, position, parents);
  klass->scope = std::make_shared<Scope>(klass);
  auto thisType = std::make_shared<Type>(klass, std::vector<uint8_t> { (uint8_t)Shared::TypeModifierFlag::Reference });
  klass->isStructure = isStructure;

  if (!isStructure) {
    klass->scope->items.push_back(std::make_shared<Variable>("this", thisType, position, klass->scope));
  }

  return klass;
};

AltaCore::DET::Class::Class(std::string _name, std::shared_ptr<AltaCore::DET::Scope> _parentScope, AltaCore::Errors::Position position, std::vector<std::shared_ptr<Class>> _parents):
  ScopeItem(_name, position, _parentScope),
  parents(_parents)
  {};

bool AltaCore::DET::Class::hasParent(std::shared_ptr<Class> parent) const {
  for (auto& myParent: parents) {
    if (myParent->id == parent->id) return true;
    if (myParent->hasParent(parent)) return true;
  }
  return false;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::DET::Class::instantiateGeneric(std::vector<std::shared_ptr<Type>> genericArguments) {
  if (auto klass = ast.lock()) {
    auto inf = info.lock();
    if (!inf) {
      return nullptr;
    }
    return klass->instantiateGeneric(inf, genericArguments);
  } else {
    return nullptr;
  }
};

namespace {
  using namespace AltaCore::DET;

  #define AC_CAST_FROM_LOOP if (from->indirectionLevel() == 0 && from->klass && onlyDo != 1) {\
      for (auto& method: from->klass->toCasts) {\
        auto& special = method->returnType;
  #define AC_CAST_FROM_LOOP_END }}
  #define AC_CAST_TO_LOOP if (to->indirectionLevel() == 0 && to->klass && onlyDo != 2) {\
      for (auto& method: to->klass->fromCasts) {\
        auto& special = method->parameterVariables.front()->type;
  #define AC_CAST_TO_LOOP_END }}

  bool doFromOrToLoop(std::shared_ptr<Type> from, std::shared_ptr<Type> to, size_t onlyDo = 0) {
    using NT = NativeType;

    // basic iteration
    {
      if (
        *from == *to ||
        *from == *to->deconstify() ||
        *from == *to->deconstify(true) ||
        *from->deconstify() == *to ||
        *from->deconstify() == *to->deconstify() ||
        *from->deconstify() == *to->deconstify(true)
      ) {
        return true;
      }
    };
    AC_CAST_FROM_LOOP;
      if (
        *special == *to ||
        *special == *to->deconstify() ||
        *special == *to->deconstify(true) ||
        *special->deconstify() == *to ||
        *special->deconstify() == *to->deconstify() ||
        *special->deconstify() == *to->deconstify(true)
      ) {
        return true;
      }
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (
        *from == *special ||
        *from == *special->deconstify() ||
        *from == *special->deconstify(true) ||
        *from->deconstify() == *special ||
        *from->deconstify() == *special->deconstify() ||
        *from->deconstify() == *special->deconstify(true)
      ) {
        return true;
      }
    AC_CAST_TO_LOOP_END;

    // floating-point iteration
    {
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
        return true;
      }
    };
    AC_CAST_FROM_LOOP;
      if (
        special->indirectionLevel() == 0 &&
        to->indirectionLevel() == 0 &&
        special->isNative &&
        to->isNative &&
        (
          special->nativeTypeName == NT::Float ||
          special->nativeTypeName == NT::Double
        ) &&
        (
          to->nativeTypeName == NT::Float ||
          to->nativeTypeName == NT::Double
        )
      ) {
        return true;
      }
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (
        from->indirectionLevel() == 0 &&
        special->indirectionLevel() == 0 &&
        from->isNative &&
        special->isNative &&
        (
          from->nativeTypeName == NT::Float ||
          from->nativeTypeName == NT::Double
        ) &&
        (
          special->nativeTypeName == NT::Float ||
          special->nativeTypeName == NT::Double
        )
      ) {
        return true;
      }
    AC_CAST_TO_LOOP_END;

    // native iteration
    {
      if (from->indirectionLevel() == 0 && to->indirectionLevel() == 0 && from->isNative && to->isNative && !from->isAny && !to->isAny) {
        return true;
      }
    };
    AC_CAST_FROM_LOOP;
      if (special->indirectionLevel() == 0 && to->indirectionLevel() == 0 && special->isNative && to->isNative && !special->isAny && !to->isAny) {
        return true;
      }
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (from->indirectionLevel() == 0 && special->indirectionLevel() == 0 && from->isNative && special->isNative && !from->isAny && !special->isAny) {
        return true;
      }
    AC_CAST_TO_LOOP_END;

    // child iteration
    {
      if (from->indirectionLevel() > 0 && to->indirectionLevel() > 0 && from->referenceLevel() == to->referenceLevel() && from->klass && to->klass && to->klass->hasParent(from->klass)) {
        return true;
      }
    };
    AC_CAST_FROM_LOOP;
      if (special->indirectionLevel() > 0 && to->indirectionLevel() > 0 && special->referenceLevel() == to->referenceLevel() && special->klass && to->klass && to->klass->hasParent(special->klass)) {
        return true;
      }
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (from->indirectionLevel() > 0 && special->indirectionLevel() > 0 && from->referenceLevel() == special->referenceLevel() && from->klass && special->klass && special->klass->hasParent(from->klass)) {
        return true;
      }
    AC_CAST_TO_LOOP_END;

    // parent iteration
    {
      if (
        (
          (
            (from->referenceLevel() <= 1 && to->referenceLevel() <= 1) ||
            (from->pointerLevel() <= 1)
          ) &&
          (to->pointerLevel() == from->pointerLevel())
        ) &&
        from->klass &&
        to->klass &&
        from->klass->hasParent(to->klass)
      ) {
        return true;
      }
    };
    AC_CAST_FROM_LOOP;
      if (
        (
          (
            (special->referenceLevel() <= 1 && to->referenceLevel() <= 1) ||
            (special->pointerLevel() <= 1)
          ) &&
          (to->pointerLevel() == special->pointerLevel())
        ) &&
        special->klass &&
        to->klass &&
        special->klass->hasParent(to->klass)
      ) {
        return true;
      }
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (
        (
          (
            (from->referenceLevel() <= 1 && special->referenceLevel() <= 1) ||
            (from->pointerLevel() <= 1)
          ) &&
          (special->pointerLevel() == from->pointerLevel())
        ) &&
        from->klass &&
        special->klass &&
        from->klass->hasParent(special->klass)
      ) {
        return true;
      }
    AC_CAST_TO_LOOP_END;

    // ref iteration
    {
      size_t maxToRefLevel = 0;
      if (to->referenceLevel() > maxToRefLevel) {
        maxToRefLevel = to->referenceLevel();
      }

      if (from->referenceLevel() < maxToRefLevel) {
        if (doFromOrToLoop(from->reference(), to)) return true;
      };
    };
    AC_CAST_FROM_LOOP;
      size_t maxToRefLevel = 0;
      if (to->referenceLevel() > maxToRefLevel) {
        maxToRefLevel = to->referenceLevel();
      }

      if (special->referenceLevel() < maxToRefLevel) {
        if (doFromOrToLoop(special->reference(), to)) return true;
      };
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      size_t maxToRefLevel = 0;
      if (special->referenceLevel() > maxToRefLevel) {
        maxToRefLevel = special->referenceLevel();
      }

      if (from->referenceLevel() < maxToRefLevel) {
        if (doFromOrToLoop(from->reference(), special)) return true;
      };
    AC_CAST_TO_LOOP_END;

    // union iteration
    {
      if (!from->isUnion() && to->isUnion() && to->indirectionLevel() == 0) {
        for (auto& otherTo: to->unionOf) {
          if (doFromOrToLoop(from, otherTo)) return true;
        }
      }
    };
    AC_CAST_FROM_LOOP;
      if (!special->isUnion() && to->isUnion() && to->indirectionLevel() == 0) {
        for (auto& otherTo: to->unionOf) {
          if (doFromOrToLoop(special, otherTo)) return true;
        }
      }
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (!from->isUnion() && special->isUnion() && special->indirectionLevel() == 0) {
        for (auto& otherTo: special->unionOf) {
          if (doFromOrToLoop(from, otherTo)) return true;
        }
      }
    AC_CAST_TO_LOOP_END;

    // nullptr iteration
    if (from->isAny && from->pointerLevel() == 1) {
      {
        if (to->pointerLevel() > 0) {
          return true;
        }
      };
      AC_CAST_TO_LOOP;
        if (special->pointerLevel() > 0) {
          return true;
        }
      AC_CAST_TO_LOOP_END;
    }

    // recursive iteration
    AC_CAST_FROM_LOOP;
      if (doFromOrToLoop(special, to)) return true;
    AC_CAST_FROM_LOOP_END;
    AC_CAST_TO_LOOP;
      if (doFromOrToLoop(from, special)) return true;
    AC_CAST_TO_LOOP_END;

    return false;
  };
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Class::findFromCast(const Type& from) {
  for (auto& fromFunc: fromCasts) {
    auto special = std::get<1>(fromFunc->parameters[0]);
    if (from == *special || from == *special->deconstify() || from == *special->deconstify(true) || *from.deconstify() == *special || *from.deconstify() == *special->deconstify()) {
      return fromFunc;
    }
  }
  for (auto& fromFunc: fromCasts) {
    auto special = std::get<1>(fromFunc->parameters[0]);
    if (doFromOrToLoop(std::make_shared<Type>(from), special)) return fromFunc;
  }
  // note that for `from` casts, we CANNOT search parents for cast methods
  // this is because we cannot automatically construct a child from a parent
  return nullptr;
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Class::findToCast(const Type& to) {
  for (auto& toFunc: toCasts) {
    auto special = toFunc->returnType;
    if (*special == to || *special == *to.deconstify() || *special == *to.deconstify(true) || *special->deconstify() == to || *special->deconstify() == *to.deconstify()) {
      return toFunc;
    }
  }
  for (auto& toFunc: toCasts) {
    auto special = toFunc->returnType;
    if (doFromOrToLoop(special, std::make_shared<Type>(to))) return toFunc;
  }
  if (to.unionOf.size() > 0) {
    for (auto& uni: to.unionOf) {
      if (auto func = findToCast(*uni)) return func;
    }
  }
  for (auto& parent: parents) {
    if (auto toCast = parent->findToCast(to)) return toCast;
  }

  return nullptr;
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Class::findOperator(const Shared::ClassOperatorType type, const Shared::ClassOperatorOrientation orient, std::shared_ptr<Type> argType) const {
  size_t highestCompat = 0;
  size_t compatIdx = SIZE_MAX;
  for (size_t i = 0; i < operators.size(); ++i) {
    auto& op = operators[i];
    if (op->operatorType != type) continue;
    if (op->orientation != orient) continue;
    if (argType) {
      auto compat = op->parameterVariables.front()->type->compatiblity(*argType);
      if (compat > highestCompat) {
        highestCompat = compat;
        compatIdx = i;
      }
    } else {
      highestCompat = SIZE_MAX;
      compatIdx = i;
      break;
    }
  }
  if (highestCompat != 0) {
    return operators[compatIdx];
  }
  for (auto& parent: parents) {
    auto func = parent->findOperator(type, orient, argType);
    if (func) return func;
  }
  return nullptr;
};

std::vector<std::shared_ptr<AltaCore::DET::Function>> AltaCore::DET::Class::findAllVirtualFunctions() {
  std::vector<std::shared_ptr<AltaCore::DET::Function>> virtFuncs;

  for (auto& item: scope->items) {
    if (item->nodeType() != NodeType::Function) continue;
    auto func = std::dynamic_pointer_cast<Function>(item);
    if (func->isVirtual()) virtFuncs.push_back(func);
  }

  for (auto& parent: parents) {
    auto otherFuncs = parent->findAllVirtualFunctions();

    for (auto& otherFunc: otherFuncs) {
      auto otherType = Type::getUnderlyingType(otherFunc);
      bool skip = false;
      for (auto& virtFunc: virtFuncs) {
        if (virtFunc->name == otherFunc->name && *Type::getUnderlyingType(virtFunc) == *otherType) {
          skip = true;
          break;
        }
      }
      if (skip) {
        continue;
      }
      virtFuncs.push_back(otherFunc);
    }
  }

  return virtFuncs;
};

std::string AltaCore::DET::Class::toString() const {
  std::string result = name;

  if (genericArguments.size() > 0) {
    result += '<';
    bool isFirst = true;
    for (auto& genArg: genericArguments) {
      if (isFirst) {
        isFirst = false;
      } else {
        result += ", ";
      }
      result += genArg->toString();
    }
    result += '>';
  }

  result = Util::joinDETPaths({ (parentScope.lock() ? parentScope.lock()->toString() : ""), result });

  return result;
};

bool AltaCore::DET::Class::isCaptureClass() const {
  if (auto pScope = parentScope.lock()) {
    while (auto ns = pScope->parentNamespace.lock()) {
      pScope = ns->parentScope.lock();
      if (!pScope) return false;
    }
    if (pScope->parentModule.lock()) {
      return false;
    }
  }
  return true;
};

auto AltaCore::DET::Class::fullPrivateHoistedItems() const -> std::vector<std::shared_ptr<ScopeItem>> {
  auto result = privateHoistedItems;

  for (auto& item: scope->items) {
    auto priv = item->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  for (auto& ctor: constructors) {
    auto priv = ctor->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  if (destructor) {
    auto priv = destructor->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  for (auto& from: fromCasts) {
    auto priv = from->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  for (auto& to: toCasts) {
    auto priv = to->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  for (auto& op: operators) {
    auto priv = op->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  return result;
};
