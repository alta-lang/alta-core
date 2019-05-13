#include "../../include/altacore/ast/floating-point-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::FloatingPointLiteralNode::nodeType() {
  return NodeType::FloatingPointLiteralNode;
};

AltaCore::AST::FloatingPointLiteralNode::FloatingPointLiteralNode(std::string _raw):
  LiteralNode(_raw)
  {};
