#include "../../include/altacore/ast/expression-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ExpressionStatement::nodeType() {
  return NodeType::ExpressionStatement;
};

AltaCore::AST::ExpressionStatement::ExpressionStatement() {};
AltaCore::AST::ExpressionStatement::ExpressionStatement(std::shared_ptr<AltaCore::AST::ExpressionNode> _expression):
  expression(_expression)
  {};


void AltaCore::AST::ExpressionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  expression->detail(scope);
};

ALTACORE_AST_VALIDATE_D(ExpressionStatement) {
  ALTACORE_VS_S;
  if (!expression) throw ValidationError("empty expression for expression statement");
  expression->validate(stack);
  ALTACORE_VS_E;
};