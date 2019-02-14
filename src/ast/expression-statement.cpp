#include "../../include/altacore/ast/expression-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ExpressionStatement::nodeType() {
  return NodeType::ExpressionStatement;
};

AltaCore::AST::ExpressionStatement::ExpressionStatement() {};
AltaCore::AST::ExpressionStatement::ExpressionStatement(std::shared_ptr<AltaCore::AST::ExpressionNode> _expression):
  expression(_expression)
  {};


ALTACORE_AST_DETAIL_D(ExpressionStatement) {
  ALTACORE_MAKE_DH(ExpressionStatement);
  info->expression = expression->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(ExpressionStatement) {
  ALTACORE_VS_S(ExpressionStatement);
  if (!expression) ALTACORE_VALIDATION_ERROR("empty expression for expression statement");
  expression->validate(stack, info->expression);
  ALTACORE_VS_E;
};
