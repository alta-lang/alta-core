#include "../include/altacore/ast/integer-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::IntegerLiteralNode::nodeType() {
  return NodeType::IntegerLiteralNode;
};

AltaCore::AST::IntegerLiteralNode::IntegerLiteralNode(std::string _raw):
  raw(_raw)
  {};