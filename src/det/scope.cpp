#include "../../include/altacore/det/scope.hpp"
#include "../../include/altacore/det/function.hpp"
#include "../../include/altacore/det/module.hpp"
#include "../../include/altacore/det/alias.hpp"
#include "../../include/altacore/det/namespace.hpp"
#include "../../include/altacore/det/variable.hpp"
#include "../../include/altacore/det/class.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Scope::nodeType() {
  return NodeType::Scope;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Scope::clone() {
  return std::make_shared<Scope>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Scope::deepClone() {
  auto self = std::dynamic_pointer_cast<Scope>(clone());
  self->items.clear();
  for (auto& item: items) {
    auto newItem = std::dynamic_pointer_cast<ScopeItem>(item->deepClone());
    newItem->parentScope = self;
    self->items.push_back(newItem);
  }
  return self;
};

auto AltaCore::DET::Scope::makeWithParentScope(std::shared_ptr<Scope> parent) -> std::shared_ptr<Scope> {
  auto scope = std::make_shared<Scope>(parent);
  parent->childScopes.push_back(scope);
  return scope;
};

AltaCore::DET::Scope::Scope() {};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Scope> _parent):
  parent(_parent),
  relativeID(_parent->nextChildID)
{
  _parent->nextChildID++;
  noRuntime = _parent->noRuntime;
};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Module> _parentModule):
  parentModule(_parentModule)
  {};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Function> _parentFunction):
  parentFunction(_parentFunction)
{
  if (auto parent = parentFunction.lock()->parentScope.lock()) {
    noRuntime = parent->noRuntime;
  }
};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Namespace> _parentNamespace):
  parentNamespace(_parentNamespace)
{
  if (auto parent = parentNamespace.lock()->parentScope.lock()) {
    noRuntime = parent->noRuntime;
  }
};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Class> _parentClass):
  parentClass(_parentClass)
{
  if (auto parent = parentClass.lock()->parentScope.lock()) {
    noRuntime = parent->noRuntime;
  }
};

std::vector<std::shared_ptr<AltaCore::DET::ScopeItem>> AltaCore::DET::Scope::findAll(std::string name, std::vector<std::shared_ptr<Type>> excludeTypes, bool searchParents, std::shared_ptr<Scope> originScope) {
  std::vector<std::shared_ptr<ScopeItem>> results;
  std::vector<std::shared_ptr<Type>> funcTypes;
  std::shared_ptr<ScopeItem> first = nullptr;
  bool allFunctions = true;

  for (auto& item: items) {
    if (item->name == name) {
      if (originScope && !originScope->canSee(item)) {
        continue;
      }
      auto trueItem = item;
      while (trueItem->nodeType() == NodeType::Alias) {
        trueItem = std::dynamic_pointer_cast<Alias>(item)->target;
      }
      if (trueItem->nodeType() != NodeType::Function || std::dynamic_pointer_cast<Function>(trueItem)->isAccessor) {
        if (allFunctions && first) {
          // we've already found a non-function scope item with that name
          throw std::runtime_error("found non-function scope item with same name as another scope item; this is a conflict");
        } else if (allFunctions && results.size() == 0) {
          // we don't have a first item yet, and allFunctions is still true and there's no functions found.
          // this means we found the first (and hopefully only) item
          first = trueItem;
        } else if (allFunctions) {
          // we've found functions with that name (since `results.size() > 0` here)
          throw std::runtime_error("found non-function scope item with same name as a function; this is a conflict");
        }
        allFunctions = false;
      } else if (!allFunctions) {
        // it is a function, but we already found a non-function item with that name
        throw std::runtime_error("found function scope item with same name as a non-function scope item; this is a conflict");
      } else {
        auto type = Type::getUnderlyingType(trueItem);
        bool ok = true;
        if (type && excludeTypes.size() > 0) {
          for (auto& excl: excludeTypes) {
            if (type->isExactlyCompatibleWith(*excl)) {
              ok = false;
              break;
            }
          }
        }
        if (ok) {
          results.push_back(trueItem);
          funcTypes.push_back(type);
        }
      }
    }
  }

  std::shared_ptr<Scope> parentScope = nullptr;
  if (!parent.expired()) {
    parentScope = parent.lock();
  } else if (!parentFunction.expired() && !parentFunction.lock()->parentScope.expired()) {
    parentScope = parentFunction.lock()->parentScope.lock();
  } else if (!parentNamespace.expired() && !parentNamespace.lock()->parentScope.expired()) {
    parentScope = parentNamespace.lock()->parentScope.lock();
  } else if (!parentClass.expired() && !parentClass.lock()->parentScope.expired()) {
    parentScope = parentClass.lock()->parentScope.lock();
  }

  if (searchParents && parentScope != nullptr && (allFunctions || !first)) {
    // here, allFunctions being true means that either all the scope items found were functions
    // (in which case, we're free to search for overloads in parent scopes)
    // OR no items were found (which is fine, too, since we can also search parent scopes in that case)
    funcTypes.insert(funcTypes.end(), excludeTypes.begin(), excludeTypes.end());
    auto otherResults = parentScope->findAll(name, funcTypes);
    if (otherResults.size() == 1) {
      if (otherResults[0]->nodeType() == NodeType::Function) {
        results.push_back(otherResults[0]);
      } else if (!first) {
        first = otherResults[0];
      }
      // otherwise, ignore the result if it's not a function
    } else {
      results.insert(results.end(), otherResults.begin(), otherResults.end());
    }
  }

  if (first) {
    return { first };
  } else {
    return results;
  }
};

void AltaCore::DET::Scope::hoist(std::shared_ptr<AltaCore::DET::ScopeItem> item) {
  if (auto ns = std::dynamic_pointer_cast<DET::Namespace>(item)) {
    if (!ns->underlyingEnumerationType)
      return;
  }
  if (item->nodeType() == NodeType::Type) {
    auto type = std::dynamic_pointer_cast<DET::Type>(item);
    if (type->name.empty()) {
      if (type->isFunction) {
        for (auto& param: type->parameters) {
          hoist(std::get<1>(param));
        }
        hoist(type->returnType);
      } else if (type->isUnion()) {
        for (auto& member: type->unionOf) {
          hoist(member);
        }
      } else if (type->isOptional) {
        hoist(type->optionalTarget);
      } else {
        if (type->klass) {
          hoist(type->klass);
        }
        return;
      }
    }
  }
  if (item->nodeType() == NodeType::Variable) {
    if (auto parent = item->parentScope.lock()) {
      if (auto func = Util::getFunction(parent).lock()) {
        return;
      } if (auto klass = parent->parentClass.lock()) {
        return;
      }
    }
  }
  // this is weird, we shouldn't need this but it covers some functions being weird
  if (auto func = std::dynamic_pointer_cast<Function>(item)) {
    for (auto& param: func->parameters) {
      if (auto type = std::get<1>(param)) {
        hoist(type);
      }
    }
    if (func->returnType) {
      hoist(func->returnType);
    }
  }
  if (auto mod = parentModule.lock()) {
    mod->hoistedItems.push_back(item);
  } else if (auto func = parentFunction.lock()) {
    func->privateHoistedItems.push_back(item);
  } else if (auto klass = parentClass.lock()) {
    klass->privateHoistedItems.push_back(item);
  } else if (auto ns = parentNamespace.lock()) {
    if (auto scope = ns->parentScope.lock()) {
      scope->hoist(item);
    } else {
      throw std::runtime_error("failed to hoist item anywhere");
    }
  } else if (auto scope = parent.lock()) {
    scope->hoist(item);
  } else {
    throw std::runtime_error("failed to hoist item anywhere");
  }
  if (auto mod = Util::getModule(this).lock()) {
    if (auto otherMod = Util::getModule(item->parentScope.lock().get()).lock()) {
      otherMod->dependents.push_back(mod);
      if (item->genericParameterCount > 0) {
        mod->genericsUsed.push_back(item);
      }
    }
  }
};

void AltaCore::DET::Scope::unhoist(std::shared_ptr<AltaCore::DET::ScopeItem> item) {
  if (item->nodeType() == NodeType::Namespace) return;
  if (auto mod = parentModule.lock()) {
    for (size_t i = 0; i < mod->hoistedItems.size(); i++) {
      if (mod->hoistedItems[i]->id == item->id) {
        mod->hoistedItems.erase(mod->hoistedItems.begin() + i);
        break;
      }
    }
  } else if (auto func = parentFunction.lock()) {
    for (size_t i = 0; i < func->privateHoistedItems.size(); i++) {
      if (func->privateHoistedItems[i]->id == item->id) {
        func->privateHoistedItems.erase(func->privateHoistedItems.begin() + i);
        break;
      }
    }
  } else if (auto klass = parentClass.lock()) {
    for (size_t i = 0; i < klass->privateHoistedItems.size(); i++) {
      if (klass->privateHoistedItems[i]->id == item->id) {
        klass->privateHoistedItems.erase(klass->privateHoistedItems.begin() + i);
        break;
      }
    }
  } else if (auto ns = parentNamespace.lock()) {
    if (auto scope = ns->parentScope.lock()) {
      scope->unhoist(item);
    }
  } else if (auto scope = parent.lock()) {
    scope->unhoist(item);
  }
  if (item->genericParameterCount > 0) {
    if (auto mod = Util::getModule(this).lock()) {
      for (size_t i = 0; i < mod->genericsUsed.size(); i++) {
        if (mod->genericsUsed[i]->id == item->id) {
          mod->genericsUsed.erase(mod->genericsUsed.begin() + i);
          break;
        }
      }
    }
  }
};

std::shared_ptr<AltaCore::DET::Scope> AltaCore::DET::Scope::getMemberScope(std::shared_ptr<AltaCore::DET::ScopeItem> item) {
  auto detType = item->nodeType();

  if (detType == NodeType::Namespace) {
    auto ns = std::dynamic_pointer_cast<Namespace>(item);
    return ns->scope;
  } else if (detType == NodeType::Function) {
    auto func = std::dynamic_pointer_cast<Function>(item);
    return func->scope;
  } else if (detType == NodeType::Variable) {
    auto var = std::dynamic_pointer_cast<Variable>(item);
    if (var->type->bitfield) return var->type->bitfield->scope;
    if (var->type->isNative || var->type->isUnion()) return nullptr;
    return var->type->klass->scope;
  }

  return nullptr;
};

bool AltaCore::DET::Scope::hasParent(std::shared_ptr<Scope> lookup) const {
  // `s` for `strong` (i.e. locked)
  if (auto sParent = parent.lock()) {
    if (sParent->id == lookup->id) return true;
    return sParent->hasParent(lookup);
  } else if (auto sModule = parentModule.lock()) {
    return false;
  } else if (auto sFunction = parentFunction.lock()) {
    if (auto sParent = sFunction->parentScope.lock()) {
      if (sParent->id == lookup->id) return true;
      return sParent->hasParent(lookup);
    }
  } else if (auto sNamespace = parentNamespace.lock()) {
    if (auto sParent = sNamespace->parentScope.lock()) {
      if (sParent->id == lookup->id) return true;
      return sParent->hasParent(lookup);
    }
  } else if (auto sClass = parentClass.lock()) {
    if (auto sParent = sClass->parentScope.lock()) {
      if (sParent->id == lookup->id) return true;
      return sParent->hasParent(lookup);
    }
  }
  return false;
};

bool AltaCore::DET::Scope::canSee(std::shared_ptr<ScopeItem> item) const {
  if (item->visibility == Visibility::Private) {
    auto itemScope = item->parentScope.lock();
    if (itemScope && id != itemScope->id && !hasParent(itemScope)) {
      return false;
    }
  } else if (item->visibility == Visibility::Protected) {
    auto itemClass = Util::getClass(item->parentScope.lock()).lock();
    auto thisClass = Util::getClass(shared_from_this()).lock();
    if (!itemClass || !thisClass) {
      return false;
    }
    if (thisClass->id != itemClass->id && !thisClass->hasParent(itemClass)) {
      return false;
    }
  } else if (item->visibility == Visibility::Module) {
    auto itemModule = Util::getModule(item->parentScope.lock().get()).lock();
    auto thisModule = Util::getModule(this).lock();
    if (!itemModule || !thisModule) {
      return false;
    }
    if (itemModule->id != thisModule->id) {
      return false;
    }
  } else if (item->visibility == Visibility::Package) {
    auto itemModule = Util::getModule(item->parentScope.lock().get()).lock();
    auto thisModule = Util::getModule(this).lock();
    if (!itemModule || !thisModule) {
      return false;
    }
    if (!(itemModule->packageInfo.root == thisModule->packageInfo.root)) {
      return false;
    }
  }
  return true;
};

std::weak_ptr<AltaCore::DET::Scope> AltaCore::DET::Scope::findTry() {
  std::weak_ptr<AltaCore::DET::Scope> result;
  if (auto scope = parent.lock()) {
    if (auto func = scope->parentFunction.lock()) {
      func->throws(true);
      scope->isTry = true;
    }
    if (scope->isTry) {
      result = scope;
      return result;
    }
    return scope->findTry();
  }
  return result;
};

void AltaCore::DET::Scope::addPossibleError(std::shared_ptr<Type> errorType) {
  if (isTry) {
    typesThrown.insert(errorType);
  } else if (auto func = parentFunction.lock()) {
    func->throws(true);
    isTry = true;
    typesThrown.insert(errorType);
  } else {
    auto tgt = findTry().lock();
    if (tgt) {
      tgt->addPossibleError(errorType);
    } else {
      throw std::runtime_error("Couldn't annotate error anywhere!");
    }
  }
};

bool AltaCore::DET::Scope::contains(std::shared_ptr<ScopeItem> item) {
  auto lockedParent = item->parentScope.lock();
  if (!lockedParent) return false;
  if (id == lockedParent->id) return true;
  return lockedParent->hasParent(shared_from_this());
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Scope::findParentLambda() {
  if (auto func = parentFunction.lock()) {
    if (func->isLambda) {
      return func;
    }
  } else if (auto scope = parent.lock()) {
    return scope->findParentLambda();
  }
  return nullptr;
};

std::string AltaCore::DET::Scope::toString() const {
  std::string result;

  result = "<scope#" + std::to_string(relativeID) + '>';

  if (auto pScope = parent.lock()) {
    result = pScope->toString() + result;
  }

  if (auto pMod = parentModule.lock()) {
    result = '[' + pMod->toString() + "]." + result;
  } else if (auto pFunc = parentFunction.lock()) {
    auto str = pFunc->toString();
    auto pos = str.find_last_of('.');
    result = str.substr(0, pos) + ".[" + str.substr(pos + 1) + "]." + result;
  } else if (auto pClass = parentClass.lock()) {
    result = pClass->toString() + '.' + result;
  } else if (auto pNamespace = parentNamespace.lock()) {
    result = pNamespace->toString() + '.' + result;
  }

  return result;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::DET::Scope::findParentCaptureClass() {
  if (auto klass = parentClass.lock()) {
    if (klass->isCaptureClass()) {
      return klass;
    }
  } else if (auto scope = parent.lock()) {
    return scope->findParentCaptureClass();
  } else if (auto func = parentFunction.lock()) {
    if (auto scope = func->parentScope.lock()) {
      return scope->findParentCaptureClass();
    }
  }
  return nullptr;
};

auto AltaCore::DET::Scope::findClosestParentScope() -> std::shared_ptr<Scope> {
  if (auto sParent = parent.lock()) {
    return sParent;
  } else if (auto sFunction = parentFunction.lock()) {
    if (auto sParent = sFunction->parentScope.lock()) {
      return sParent;
    }
  } else if (auto sNamespace = parentNamespace.lock()) {
    if (auto sParent = sNamespace->parentScope.lock()) {
      return sParent;
    }
  } else if (auto sClass = parentClass.lock()) {
    if (auto sParent = sClass->parentScope.lock()) {
      return sParent;
    }
  }
  return nullptr;
};
