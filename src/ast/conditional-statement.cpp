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
