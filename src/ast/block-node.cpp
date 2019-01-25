#include "../../include/altacore/ast/block-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::BlockNode::nodeType() {
  return NodeType::BlockNode;
};

AltaCore::AST::BlockNode::BlockNode() {};
AltaCore::AST::BlockNode::BlockNode(std::vector<std::shared_ptr<AltaCore::AST::StatementNode>> _statements):
  statements(_statements)
  {};

void AltaCore::AST::BlockNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  for (auto& stmt: statements) {
    stmt->detail(scope);
  }
};

ALTACORE_AST_VALIDATE_D(BlockNode) {
  
};