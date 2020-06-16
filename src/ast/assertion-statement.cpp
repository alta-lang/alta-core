#include "../../include/altacore/ast/assertion-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AssertionStatement::nodeType() {
  return NodeType::AssertionStatement;
};

ALTACORE_AST_DETAIL_D(AssertionStatement) {
  ALTACORE_MAKE_DH(AssertionStatement);

  info->test = test->fullDetail(info->inputScope);

  return info;
};

ALTACORE_AST_VALIDATE_D(AssertionStatement) {
  ALTACORE_VS_S(AssertionStatement);

  test->validate(stack, info->test);

  ALTACORE_VS_E;
};
