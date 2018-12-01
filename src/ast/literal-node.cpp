#include "../../include/altacore/ast/literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::LiteralNode::nodeType() {
  return NodeType::LiteralNode;
};

AltaCore::AST::LiteralNode::LiteralNode(std::string _raw):
  raw(_raw)
  {};
