#include "../../include/altacore/ast/return-directive-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ReturnDirectiveNode::nodeType() {
  return NodeType::ReturnDirectiveNode;
};

AltaCore::AST::ReturnDirectiveNode::ReturnDirectiveNode(std::shared_ptr<AltaCore::AST::ExpressionNode> _expression):
  expression(_expression)
  {};

void AltaCore::AST::ReturnDirectiveNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  if (expression != nullptr) {
    return expression->detail(scope);
  }
};

ALTACORE_AST_VALIDATE_D(ReturnDirectiveNode) {
  ALTACORE_VS_S;
  if (expression) {
    expression->validate(stack);
  }
  ALTACORE_VS_E;
};