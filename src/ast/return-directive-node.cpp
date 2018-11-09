#include "../include/altacore/ast/return-directive-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ReturnDirectiveNode::nodeType() {
  return NodeType::ReturnDirectiveNode;
};

AltaCore::AST::ReturnDirectiveNode::ReturnDirectiveNode(AltaCore::AST::ExpressionNode* _expression):
  expression(_expression)
  {};

void AltaCore::AST::ReturnDirectiveNode::detail(AltaCore::DET::Scope* scope) {
  if (expression != nullptr) {
    return expression->detail(scope);
  }
};