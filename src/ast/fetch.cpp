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
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  size_t highestCompat = 0;
  for (auto& item: info->items) {
    auto itemType = DET::Type::getUnderlyingType(item);
    auto compat = itemType->compatiblity(*type);
    if (compat > highestCompat) {
      highestCompat = compat;
      info->narrowedTo = item;
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
void AltaCore::AST::Fetch::narrowTo(std::shared_ptr<DH::Fetch> info, size_t i) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
  }
  info->narrowedTo = info->items[i];
  if (info->narrowedTo) {
    if (auto parentScope = info->narrowedTo->parentScope.lock()) {
      if (auto parentModule = parentScope->parentModule.lock()) {
        info->inputScope->hoist(info->narrowedTo);
      }
    }
  }
};
void AltaCore::AST::Fetch::widen(std::shared_ptr<DH::Fetch> info) {
  if (info->narrowedTo) {
    info->inputScope->unhoist(info->narrowedTo);
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

  if (items.size() == 1) {
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
  ALTACORE_VS_E;
};
