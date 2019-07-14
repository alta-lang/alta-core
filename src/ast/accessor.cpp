#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/det.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Accessor::nodeType() {
  return NodeType::Accessor;
};

AltaCore::AST::Accessor::Accessor(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::string _query):
  RetrievalNode(_query),
  target(_target)
  {};

ALTACORE_AST_DETAIL_D(Accessor) {
  ALTACORE_MAKE_DH(Accessor);

  info->target = target->fullDetail(scope);

  std::shared_ptr<DET::Scope> targetScope = nullptr;
  auto targetAcc = std::dynamic_pointer_cast<Accessor>(target);
  auto targetAccDH = std::dynamic_pointer_cast<DH::Accessor>(info->target);
  bool notAccessingNamespace = false;
  
  if (auto retr = std::dynamic_pointer_cast<Fetch>(target)) {
    auto retrInfo = std::dynamic_pointer_cast<DH::Fetch>(info->target);
    if (retrInfo->narrowedTo) {
      if (auto var = std::dynamic_pointer_cast<DET::Variable>(retrInfo->narrowedTo)) {
        if (var->isVariable) {
          info->getsVariableLength = true;
          return info;
        }
      }
    }
  }

  if (targetAcc && targetAccDH->readAccessor) {
    if (targetAccDH->readAccessor->returnType->bitfield) {
      targetScope = targetAccDH->readAccessor->returnType->bitfield->scope;
    } else {
      if ((targetAccDH->readAccessor->returnType->isNative) || targetAccDH->readAccessor->returnType->isUnion()) {
        ALTACORE_DETAILING_ERROR("native types can't be accessed");
      }
      targetScope = targetAccDH->readAccessor->returnType->klass->scope;
    }
  } else {
    auto items = DET::ScopeItem::getUnderlyingItems(info->target);

    if (items.size() == 1) {
      if (items[0]->nodeType() == DET::NodeType::Function) {
        ALTACORE_DETAILING_ERROR("can't access a function");
      } else if (items[0]->nodeType() == DET::NodeType::Namespace) {
        info->accessesNamespace = true;
      }
      try {
        info->targetType = DET::Type::getUnderlyingType(items[0]);
      } catch (...) {
        info->targetType = nullptr;
      }
      targetScope = DET::Scope::getMemberScope(items[0]);
    } else if (items.size() > 0) {
      ALTACORE_DETAILING_ERROR("target must be narrowed before it can be accessed");
    } else if (auto sup = std::dynamic_pointer_cast<DH::SuperClassFetch>(info->target)) {
      targetScope = sup->superclass->scope;
      notAccessingNamespace = true;
    } else {
      try {
        auto types = DET::Type::getUnderlyingTypes(info->target.get());
        if (types.size() == 1) {
          info->targetType = types[0];
          if (types[0]->bitfield) {
            targetScope = types[0]->bitfield->scope;
          } else {
            if (types[0]->isNative || types[0]->isUnion()) {
              ALTACORE_DETAILING_ERROR("native types can't be accessed");
            }
            targetScope = types[0]->klass->scope;
          }
        } else if (items.size() > 0) {
          ALTACORE_DETAILING_ERROR("target must be narrowed before it can be accessed");
        } else {
          // the `!targetScope` check will take care of this later
        }
      } catch (...) {
        // do nothing
      }
    }
  }

  if (!targetScope) {
    ALTACORE_DETAILING_ERROR("could not determine how to access the given target");
  }

  if (!targetScope->parentNamespace.expired() && !notAccessingNamespace) {
    info->accessesNamespace = true;
  }

  info->items = targetScope->findAll(query, {}, false, scope);

  /*
   * search parent classes
   *
   * this isn't done as a recursive lambda beacause that could
   * lead to a stack overflow. granted, the number of parent classes
   * a class would have to have to cause that would be absurd, but
   * nontheless possible. if we *can* handle it properly, why not?
   */
  std::vector<std::shared_ptr<DET::Class>> currentParents;
  std::stack<std::shared_ptr<DET::Class>> classStack;
  std::stack<size_t> idxs;
  classStack.push(targetScope->parentClass.lock());
  idxs.push(-1);
  while (classStack.size() > 0 && classStack.top()) {
    auto& pc = classStack.top();
    auto& idx = idxs.top();
    idx++;
    if (info->items.size() < 1 || (info->items.size() > 0 && info->items.front()->nodeType() == DET::NodeType::Function)) {
      bool loopBack = false;
      for (size_t i = idx; i < pc->parents.size(); i++) {
        auto& parent = pc->parents[i];
        currentParents.push_back(parent);
        auto stuff = parent->scope->findAll(query, {}, false, scope);
        for (size_t j = 0; j < stuff.size(); j++) {
          info->parentClassAccessors[info->items.size()] = currentParents;
          info->items.push_back(stuff[j]);
        }
        classStack.push(parent);
        idxs.push(-1);
        loopBack = true;
        break;
      }
      if (loopBack) {
        continue;
      }
    }
    classStack.pop();
    idxs.pop();
    if (currentParents.size() > 0) {
      currentParents.pop_back();
    }
  }

  bool allAccessors = true;

  for (size_t i = 0; i < info->items.size(); i++) {
    auto& item = info->items[i];
    if (item->nodeType() == DET::NodeType::Function && std::dynamic_pointer_cast<DET::Function>(item)->isAccessor) {
      auto acc = std::dynamic_pointer_cast<DET::Function>(item);
      if (acc->parameters.size() == 0) {
        if (info->readAccessor) ALTACORE_DETAILING_ERROR("encountered two read accessors with the same name");
        info->readAccessor = acc;
        info->readAccessorIndex = i;
      } else if (acc->parameters.size() == 1) {
        if (info->writeAccessor) ALTACORE_DETAILING_ERROR("encountered two write accessors with the same name");
        info->writeAccessor = acc;
        info->writeAccessorIndex = i;
      } else {
        ALTACORE_DETAILING_ERROR("invalid accessor");
      }
    }
  }

  if (info->items.size() == 0) {
    ALTACORE_DETAILING_ERROR("no items found for `" + query + "` in target");
  } else if (info->items.size() == 1) {
    if (info->items[0]->nodeType() != DET::NodeType::Function || !std::dynamic_pointer_cast<DET::Function>(info->items[0])->isAccessor) {
      narrowTo(info, 0);
    }
  }

  for (size_t i = 0; i < genericArguments.size(); i++) {
    auto& arg = genericArguments[i];
    auto det = arg->fullDetail(scope);
    info->genericArgumentDetails.push_back(det);
    if (!det->type) {
      ALTACORE_DETAILING_ERROR("failed to detail generic argument as a type");
    }
    info->genericArguments.push_back(det->type);
  }

  if (genericArguments.size() > 0) {
    for (size_t i = 0; i < info->items.size(); i++) {
      auto& item = info->items[i];
      auto type = item->nodeType();
      bool isNarrowedTo = false;

      if (info->narrowedTo && info->narrowedTo->id == info->items[i]->id) {
        isNarrowedTo = true;
      }

      if (type != DET::NodeType::Function && type != DET::NodeType::Class) {
        ALTACORE_DETAILING_ERROR(
          std::string("only functions and classes can be generic") +
          ((info->items.size() > 1) ? "(try narrowing the retrieval)" : "")
        );
      }

      // unequal number of generics = incompatible item; remove it
      if (item->genericParameterCount < genericArguments.size()) {
        info->items.erase(info->items.begin() + i);
        if (isNarrowedTo) {
          widen(info);
        }
        i--; // recheck this index since we shrunk the vector
        continue;
      }

      if (auto klass = std::dynamic_pointer_cast<DET::Class>(item)) {
        auto newKlass = klass->instantiateGeneric(info->genericArguments);
        info->items[i] = newKlass;
        auto thisMod = Util::getModule(info->inputScope.get()).lock();
        //thisMod->genericsUsed.push_back(newKlass);
        auto thatMod = Util::getModule(newKlass->parentScope.lock().get()).lock();
        if (thisMod->packageInfo.name == thatMod->packageInfo.name) {
          newKlass->instantiatedFromSamePackage = true;
        }
        info->inputScope->hoist(info->items[i]);
        if (isNarrowedTo) {
          narrowTo(info, i);
        }
      } else if (auto func = std::dynamic_pointer_cast<DET::Function>(item)) {
        auto newFunc = func->instantiateGeneric(info->genericArguments);
        info->items[i] = newFunc;
        auto thisMod = Util::getModule(info->inputScope.get()).lock();
        //thisMod->genericsUsed.push_back(newFunc);
        auto thatMod = Util::getModule(newFunc->parentScope.lock().get()).lock();
        if (thisMod->packageInfo.name == thatMod->packageInfo.name) {
          newFunc->instantiatedFromSamePackage = true;
        }
        info->inputScope->hoist(info->items[i]);
        if (isNarrowedTo) {
          narrowTo(info, i);
        }
      } else {
        ALTACORE_DETAILING_ERROR("generic type wasn't a class or function (btw, this is impossible)");
      }
    }
  }

  return info;
};

void AltaCore::AST::Accessor::narrowTo(std::shared_ptr<DH::Accessor> info, std::shared_ptr<AltaCore::DET::Type> type) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  size_t highestCompat = 0;
  for (size_t i = 0; i < info->items.size(); i++) {
    auto& item = info->items[i];
    auto itemType = DET::Type::getUnderlyingType(item);
    auto compat = itemType->compatiblity(*type);
    if (compat > highestCompat) {
      if (item->nodeType() != DET::NodeType::Function || !std::dynamic_pointer_cast<DET::Function>(item)->isAccessor) {
        highestCompat = compat;
        info->narrowedTo = item;
        info->narrowedToIndex = i;
      }
    }
  }
  if (info->narrowedTo) {
    if (auto parentScope = info->narrowedTo->parentScope.lock()) {
      if (auto parentModule = parentScope->parentModule.lock()) {
        info->inputScope->hoist(info->narrowedTo);
      }
    }
  }
};
void AltaCore::AST::Accessor::narrowTo(std::shared_ptr<DH::Accessor> info, size_t i) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  info->narrowedTo = info->items[i];
  info->narrowedToIndex = i;
  if (info->narrowedTo) {
    if (auto parentScope = info->narrowedTo->parentScope.lock()) {
      if (auto parentModule = parentScope->parentModule.lock()) {
        info->inputScope->hoist(info->narrowedTo);
      }
    }
  }
};
void AltaCore::AST::Accessor::widen(std::shared_ptr<DH::Accessor> info) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  info->narrowedTo = nullptr;
  info->narrowedToIndex = SIZE_MAX;
};

ALTACORE_AST_VALIDATE_D(Accessor) {
  ALTACORE_VS_S(Accessor);
  target->validate(stack, info->target);
  if (query.empty()) {
    ALTACORE_VALIDATION_ERROR("accessor query can't be empty");
  }
  ALTACORE_VS_E;
};
