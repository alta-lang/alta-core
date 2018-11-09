#include "../include/altacore/ast/expression-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ExpressionStatement::nodeType() {
  return NodeType::ExpressionStatement;
};

AltaCore::AST::ExpressionStatement::ExpressionStatement() {};
AltaCore::AST::ExpressionStatement::ExpressionStatement(AltaCore::AST::ExpressionNode* _expression):
  expression(_expression)
  {};


void AltaCore::AST::ExpressionStatement::detail(AltaCore::DET::Scope* scope) {
  expression->detail(scope);
};