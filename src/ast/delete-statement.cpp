#include "../../include/altacore/ast/delete-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::DeleteStatement::nodeType() {
  return NodeType::DeleteStatement;
};

ALTACORE_AST_DETAIL_D(DeleteStatement) {
  ALTACORE_MAKE_DH(DeleteStatement);

  info->target = target->fullDetail(info->inputScope);
  info->persistent = persistent;

  return info;
};

ALTACORE_AST_VALIDATE_D(DeleteStatement) {
  ALTACORE_VS_S(DeleteStatement);

  target->validate(stack, info->target);

  ALTACORE_VS_E;
};
