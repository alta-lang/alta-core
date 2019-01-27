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
  if (!primaryTest) throw ValidationError("empty primary test for conditional statement");
  if (!primaryResult) throw ValidationError("empty primary result for conditional statement");
  primaryTest->validate(stack);
  primaryResult->validate(stack);
  for (auto& [test, result]: alternatives) {
    if (!test) throw ValidationError("empty alternative test for conditional statement");
    if (!result) throw ValidationError("empty alternative result conditional statement");
    test->validate(stack);
    result->validate(stack);
  }
  if (finalResult) {
    finalResult->validate(stack);
  }
  if (!$primaryScope) throw ValidationError("improperly detailed primary scope for conditional statement");
  for (auto& scope: $alternativeScopes) {
    if (!scope) throw ValidationError("improperly detailed alternative scope for conditional statement");
  }
  if (finalResult && !$finalScope) throw ValidationError("improperly detailed final scope for conditional statement");
  ALTACORE_VS_E;
};