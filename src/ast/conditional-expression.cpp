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
