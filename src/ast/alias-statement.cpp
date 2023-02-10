#include "../../include/altacore/ast/alias-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AliasStatement::nodeType() {
  return NodeType::AliasStatement;
};

ALTACORE_AST_DETAIL_D(AliasStatement) {
  ALTACORE_MAKE_DH(AliasStatement);

  info->target = target->fullDetail(info->inputScope);

  for (auto& item: info->target->items) {
    auto alias = std::make_shared<DET::Alias>(name, item, position, info->inputScope);
    info->inputScope->items.push_back(alias);
    info->aliases.push_back(alias);
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(AliasStatement) {
  ALTACORE_VS_S(AliasStatement);
  target->validate(stack, info->target);
  ALTACORE_VS_E;
};
