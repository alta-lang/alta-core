#include "../../include/altacore/ast/code-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::CodeLiteralNode::nodeType() {
  return NodeType::CodeLiteralNode;
};

AltaCore::AST::CodeLiteralNode::CodeLiteralNode(std::string _raw):
  raw(_raw)
  {};
