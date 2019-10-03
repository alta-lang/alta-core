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

ALTACORE_AST_DETAIL_D(ConditionalStatement) {
  ALTACORE_MAKE_DH(ConditionalStatement);
  info->primaryTest = primaryTest->fullDetail(scope);
  info->primaryScope = DET::Scope::makeWithParentScope(scope);
  info->primaryResult = primaryResult->fullDetail(info->primaryScope);

  for (auto& [altTest, altResult]: alternatives) {
    auto testDet = altTest->fullDetail(scope);
    auto& altScope = info->alternativeScopes.emplace_back(scope);
    auto resDet = altResult->fullDetail(altScope);
    info->alternatives.emplace_back(testDet, resDet);
  }

  if (finalResult) {
    info->finalScope = DET::Scope::makeWithParentScope(scope);
    info->finalResult = finalResult->fullDetail(info->finalScope);
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(ConditionalStatement) {
  ALTACORE_VS_S(ConditionalStatement);
  if (!primaryTest) ALTACORE_VALIDATION_ERROR("empty primary test for conditional statement");
  if (!primaryResult) ALTACORE_VALIDATION_ERROR("empty primary result for conditional statement");
  primaryTest->validate(stack, info->primaryTest);
  primaryResult->validate(stack, info->primaryResult);
  for (size_t i = 0; i < alternatives.size(); i++) {
    auto& [test, result] = alternatives[i];
    auto [testDet, resultDet] = info->alternatives[i];
    if (!test) ALTACORE_VALIDATION_ERROR("empty alternative test for conditional statement");
    if (!result) ALTACORE_VALIDATION_ERROR("empty alternative result conditional statement");
    test->validate(stack, testDet);
    result->validate(stack, resultDet);
  }
  if (finalResult) {
    finalResult->validate(stack, info->finalResult);
  }
  if (!info->primaryScope) ALTACORE_VALIDATION_ERROR("improperly detailed primary scope for conditional statement");
  for (auto& scope: info->alternativeScopes) {
    if (!scope) ALTACORE_VALIDATION_ERROR("improperly detailed alternative scope for conditional statement");
  }
  if (finalResult && !info->finalScope) ALTACORE_VALIDATION_ERROR("improperly detailed final scope for conditional statement");
  ALTACORE_VS_E;
};
