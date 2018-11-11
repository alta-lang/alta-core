#include "../../include/altacore/ast/assignment-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AssignmentExpression::nodeType() {
  return NodeType::AssignmentExpression;
};

AltaCore::AST::AssignmentExpression::AssignmentExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::shared_ptr<AltaCore::AST::ExpressionNode> _value):
  target(_target),
  value(_value)
  {};

void AltaCore::AST::AssignmentExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);
  value->detail(scope);
};
