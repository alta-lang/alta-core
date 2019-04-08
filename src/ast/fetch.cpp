#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Fetch::nodeType() {
  return NodeType::Fetch;
};

AltaCore::AST::Fetch::Fetch(std::string _query):
  RetrievalNode(_query)
  {};

void AltaCore::AST::Fetch::narrowTo(std::shared_ptr<DH::Fetch> info, std::shared_ptr<AltaCore::DET::Type> type) {
  size_t highestCompat = 0;
  for (auto& item: info->items) {
    auto itemType = DET::Type::getUnderlyingType(item);
    auto compat = itemType->compatiblity(*type);
    if (compat > highestCompat) {
      highestCompat = compat;
      info->narrowedTo = item;
    }
  }
};

ALTACORE_AST_DETAIL_D(Fetch) {
  ALTACORE_MAKE_DH(Fetch);

  auto items = scope->findAll(query, {}, true, scope);

  if (items.size() == 1) {
    info->narrowedTo = items[0];
  }

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

      if (info->narrowedTo == info->items[i]) {
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
          info->narrowedTo = nullptr;
        }
        i--; // recheck this index since we shrunk the vector
        continue;
      }

      if (auto klass = std::dynamic_pointer_cast<DET::Class>(item)) {
        info->items[i] = klass->instantiateGeneric(info->genericArguments);
        if (isNarrowedTo) {
          info->narrowedTo = info->items[i];
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
