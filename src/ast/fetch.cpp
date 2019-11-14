#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Fetch::nodeType() {
  return NodeType::Fetch;
};

AltaCore::AST::Fetch::Fetch(std::string _query):
  RetrievalNode(_query)
  {};

void AltaCore::AST::Fetch::narrowTo(std::shared_ptr<DH::Fetch> info, std::shared_ptr<AltaCore::DET::Type> type) {
  size_t highestCompat = 0;
  size_t idx = 0;
  for (size_t i = 0; i < info->items.size(); i++) {
    auto& item = info->items[i];
    auto itemType = DET::Type::getUnderlyingType(item);
    auto compat = itemType->compatiblity(*type);
    if (compat > highestCompat) {
      highestCompat = compat;
      idx = i;
    }
  }
  narrowTo(info, idx);
};
void AltaCore::AST::Fetch::narrowTo(std::shared_ptr<DH::Fetch> info, size_t i) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  info->narrowedTo = info->items[i];
  if (info->narrowedTo->nodeType() == DET::NodeType::Variable) {
    if (auto func = Util::getFunction(info->inputScope).lock()) {
      if (func->isGenerator) {
        if (func->scope->contains(info->narrowedTo)) {
          info->referencesInsideGenerator = true;
        }
      }
    }
    auto scope = info->inputScope;
    while (scope && !scope->contains(info->narrowedTo)) {
      if (auto func = scope->parentFunction.lock()) {
        if (func->isLambda) {
          auto lambdaModule = Util::getModule(func->parentScope.lock().get()).lock();
          auto itemModule = Util::getModule(info->narrowedTo->parentScope.lock().get()).lock();
          if (lambdaModule.get() == itemModule.get()) {
            bool found = false;
            for (auto& var: func->referencedVariables) {
              if (var->id == info->narrowedTo->id) {
                found = true;
                break;
              }
            }
            if (!found) {
              func->referencedVariables.push_back(std::dynamic_pointer_cast<DET::Variable>(info->narrowedTo));
            }
            info->referencesOutsideLambda = true;
          }
        }
      }
      if (auto klass = scope->parentClass.lock()) {
        if (klass->isCaptureClass()) {
          auto klassModule = Util::getModule(klass->parentScope.lock().get()).lock();
          auto itemModule = Util::getModule(info->narrowedTo->parentScope.lock().get()).lock();
          if (klassModule.get() == itemModule.get()) {
            bool found = false;
            for (auto& var: klass->referencedVariables) {
              if (var->id == info->narrowedTo->id) {
                found = true;
                break;
              }
            }
            if (!found) {
              klass->referencedVariables.push_back(std::dynamic_pointer_cast<DET::Variable>(info->narrowedTo));
            }
            info->referencesOutsideCaptureClass = true;
          }
        }
      }
      scope = scope->findClosestParentScope();
    }
  }
  if (info->narrowedTo) {
    info->inputScope->hoist(info->narrowedTo);
  }
};
void AltaCore::AST::Fetch::widen(std::shared_ptr<DH::Fetch> info) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  auto scope = info->inputScope;
  while (scope && !scope->contains(info->narrowedTo)) {
    if (auto func = scope->parentFunction.lock()) {
      if (func->isLambda) {
        for (size_t i = 0; i < func->referencedVariables.size(); i++) {
          if (func->referencedVariables[i]->id == info->narrowedTo->id) {
            func->referencedVariables.erase(func->referencedVariables.begin() + i);
            info->referencesOutsideLambda = false;
          }
        }
      }
    }
    if (auto klass = scope->parentClass.lock()) {
      if (klass->isCaptureClass()) {
        for (size_t i = 0; i < klass->referencedVariables.size(); i++) {
          if (klass->referencedVariables[i]->id == info->narrowedTo->id) {
            klass->referencedVariables.erase(klass->referencedVariables.begin() + i);
            info->referencesOutsideCaptureClass = false;
          }
        }
      }
    }
    scope = scope->findClosestParentScope();
  }
  info->narrowedTo = nullptr;
};

ALTACORE_AST_DETAIL_D(Fetch) {
  ALTACORE_MAKE_DH(Fetch);

  auto items = scope->findAll(query, {}, true, scope);

  if (items.size() < 1) {
    // nothing was found for our query, throw an error.
    // TODO: throw it politely. i.e. through a logger.
    //       that, though, will have to wait until we implement
    //       a good logger for the whole core functionality (lexer, parser, AST,
    //       DET, module system, etc.). that way, we'll give users
    //       an easy to use commmon interface for handling our errors
    ALTACORE_DETAILING_ERROR("there's no `" + query + "` in the scope");
  }

  info->items = items;

  for (size_t i = 0; i < info->items.size(); i++) {
    auto& item = info->items[i];
    if (item->nodeType() == DET::NodeType::Function && std::dynamic_pointer_cast<DET::Function>(item)->isAccessor) {
      auto acc = std::dynamic_pointer_cast<DET::Function>(item);
      if (acc->parameters.size() == 0) {
        if (info->readAccessor) ALTACORE_DETAILING_ERROR("encountered two read accessors with the same name");
        info->readAccessor = acc;
        info->readAccessorIndex = i;
        info->inputScope->hoist(info->readAccessor);
      } else if (acc->parameters.size() == 1) {
        if (info->writeAccessor) ALTACORE_DETAILING_ERROR("encountered two write accessors with the same name");
        info->writeAccessor = acc;
        info->writeAccessorIndex = i;
        info->inputScope->hoist(info->writeAccessor);
      } else {
        ALTACORE_DETAILING_ERROR("invalid accessor");
      }
    }
  }

  if (items.size() == 1 && (info->items[0]->nodeType() != DET::NodeType::Function || !std::dynamic_pointer_cast<DET::Function>(info->items[0])->isAccessor)) {
    narrowTo(info, 0);
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

ALTACORE_AST_VALIDATE_D(Fetch) {
  ALTACORE_VS_S(Fetch);
  if (query.empty()) ALTACORE_VALIDATION_ERROR("empty query for fetch");
  if (info->items.size() < 1) ALTACORE_VALIDATION_ERROR("no items found for fetch");
  if (info->narrowedTo && info->narrowedTo->parentScope.lock() && info->narrowedTo->parentScope.lock()->parentClass.lock()) {
    if (auto var = std::dynamic_pointer_cast<DET::Variable>(info->narrowedTo)) {
      if (var->name != "this") {
        ALTACORE_VALIDATION_ERROR("Class members must be accessed through `this`");
      }
    } else if (auto func = std::dynamic_pointer_cast<DET::Function>(info->narrowedTo)) {
      ALTACORE_VALIDATION_ERROR("Class methods must be accessed through `this`");
    }
  }
  ALTACORE_VS_E;
};
