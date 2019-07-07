#include "../../include/altacore/ast/integer-literal-node.hpp"
#include <string>

const AltaCore::AST::NodeType AltaCore::AST::IntegerLiteralNode::nodeType() {
  return NodeType::IntegerLiteralNode;
};

AltaCore::AST::IntegerLiteralNode::IntegerLiteralNode(std::string _raw):
  LiteralNode(_raw),
  integer(parseInteger(_raw))
  {};
