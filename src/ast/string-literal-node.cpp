#include "../../include/altacore/ast/string-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::StringLiteralNode::nodeType() {
  return NodeType::StringLiteralNode;
};

AltaCore::AST::StringLiteralNode::StringLiteralNode(std::string _value):
  value(_value)
  {};
