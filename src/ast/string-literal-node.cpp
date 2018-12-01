#include "../../include/altacore/ast/string-literal-node.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::StringLiteralNode::nodeType() {
  return NodeType::StringLiteralNode;
};

AltaCore::AST::StringLiteralNode::StringLiteralNode(std::string _value):
  LiteralNode('"' + AltaCore::Util::escape(_value) + '"'),
  value(_value)
  {};
