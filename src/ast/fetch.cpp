#include "../../include/altacore/ast/fetch.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Fetch::nodeType() {
  return NodeType::Fetch;
};

AltaCore::AST::Fetch::Fetch(std::string _query):
  query(_query)
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

  return info;
};

ALTACORE_AST_VALIDATE_D(Fetch) {
  ALTACORE_VS_S(Fetch);
  if (query.empty()) ALTACORE_VALIDATION_ERROR("empty query for fetch");
  if (info->items.size() < 1) ALTACORE_VALIDATION_ERROR("no items found for fetch");
  ALTACORE_VS_E;
};
