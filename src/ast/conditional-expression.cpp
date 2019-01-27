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

void AltaCore::AST::ConditionalExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  test->detail(scope);
  primaryResult->detail(scope);
  secondaryResult->detail(scope);
};

ALTACORE_AST_VALIDATE_D(ConditionalExpression) {
  ALTACORE_VS_S;
  if (!test) throw ValidationError("empty test for conditional expression");
  if (!primaryResult) throw ValidationError("empty primary result for conditional expression");
  if (!secondaryResult) throw ValidationError("empty secondary result for conditional expression");
  test->validate(stack);
  primaryResult->validate(stack);
  secondaryResult->validate(stack);
  ALTACORE_VS_E;
};