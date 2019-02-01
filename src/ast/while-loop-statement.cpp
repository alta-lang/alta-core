#include "../../include/altacore/ast/while-loop-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::WhileLoopStatement::nodeType() {
  return NodeType::WhileLoopStatement;
};

void AltaCore::AST::WhileLoopStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  $scope = std::make_shared<DET::Scope>(scope);

  test->detail($scope);
  body->detail($scope);
};

ALTACORE_AST_VALIDATE_D(WhileLoopStatement) {
  ALTACORE_VS_S;
  if (!test) ALTACORE_VALIDATION_ERROR("empty test for while loop");
  test->validate(stack);
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for while loop");
  body->validate(stack);
  ALTACORE_VS_E;
};
