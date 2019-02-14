#include "../../include/altacore/ast/conditional-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ConditionalExpression::nodeType() {
  return NodeType::ConditionalExpression;
};

AltaCore::AST::ConditionalExpression::ConditionalExpression(
  std::shared_ptr<ExpressionNode> _test,
  std::shared_ptr<ExpressionNode> _primary,
  std::shared_ptr<ExpressionNode> _secondary
):
  test(_test),
  primaryResult(_primary),
  secondaryResult(_secondary)
  {};

ALTACORE_AST_DETAIL_D(ConditionalExpression) {
  ALTACORE_MAKE_DH(ConditionalExpression);
  info->test = test->fullDetail(scope);
  info->primaryResult = primaryResult->fullDetail(scope);
  info->secondaryResult = secondaryResult->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(ConditionalExpression) {
  ALTACORE_VS_S(ConditionalExpression);
  if (!test) ALTACORE_VALIDATION_ERROR("empty test for conditional expression");
  if (!primaryResult) ALTACORE_VALIDATION_ERROR("empty primary result for conditional expression");
  if (!secondaryResult) ALTACORE_VALIDATION_ERROR("empty secondary result for conditional expression");
  test->validate(stack, info->test);
  primaryResult->validate(stack, info->primaryResult);
  secondaryResult->validate(stack, info->secondaryResult);
  ALTACORE_VS_E;
};
