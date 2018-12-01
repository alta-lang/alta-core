#include "../../include/altacore/ast/boolean-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::BooleanLiteralNode::nodeType() {
  return NodeType::BooleanLiteralNode;
};

AltaCore::AST::BooleanLiteralNode::BooleanLiteralNode(bool _value):
  LiteralNode(_value ? "true" : "false"),
  value(_value)
  {};
