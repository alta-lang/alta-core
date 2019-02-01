#include "../../include/altacore/ast/conditional-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ConditionalStatement::nodeType() {
  return NodeType::ConditionalStatement;
};

AltaCore::AST::ConditionalStatement::ConditionalStatement(
  std::shared_ptr<ExpressionNode> _test,
  std::shared_ptr<StatementNode> _primary,
  std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> _alternatives,
  std::shared_ptr<StatementNode> _finalResult
):
  primaryTest(_test),
  primaryResult(_primary),
  alternatives(_alternatives),
  finalResult(_finalResult)
  {};

void AltaCore::AST::ConditionalStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  primaryTest->detail(scope);
  $primaryScope = std::make_shared<DET::Scope>(scope);
  primaryResult->detail($primaryScope);

  for (auto& [altTest, altResult]: alternatives) {
    altTest->detail(scope);
    auto& altScope = $alternativeScopes.emplace_back(scope);
    altResult->detail(altScope);
  }

  if (finalResult) {
    $finalScope = std::make_shared<DET::Scope>(scope);
    finalResult->detail($finalScope);
  }
};

ALTACORE_AST_VALIDATE_D(ConditionalStatement) {
  ALTACORE_VS_S;
  if (!primaryTest) ALTACORE_VALIDATION_ERROR("empty primary test for conditional statement");
  if (!primaryResult) ALTACORE_VALIDATION_ERROR("empty primary result for conditional statement");
  primaryTest->validate(stack);
  primaryResult->validate(stack);
  for (auto& [test, result]: alternatives) {
    if (!test) ALTACORE_VALIDATION_ERROR("empty alternative test for conditional statement");
    if (!result) ALTACORE_VALIDATION_ERROR("empty alternative result conditional statement");
    test->validate(stack);
    result->validate(stack);
  }
  if (finalResult) {
    finalResult->validate(stack);
  }
  if (!$primaryScope) ALTACORE_VALIDATION_ERROR("improperly detailed primary scope for conditional statement");
  for (auto& scope: $alternativeScopes) {
    if (!scope) ALTACORE_VALIDATION_ERROR("improperly detailed alternative scope for conditional statement");
  }
  if (finalResult && !$finalScope) ALTACORE_VALIDATION_ERROR("improperly detailed final scope for conditional statement");
  ALTACORE_VS_E;
};
