#include "../../include/altacore/ast/code-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::CodeLiteralNode::nodeType() {
  return NodeType::CodeLiteralNode;
};

AltaCore::AST::CodeLiteralNode::CodeLiteralNode(std::string _raw):
  raw(_raw)
  {};

ALTACORE_AST_DETAIL_D(CodeLiteralNode) {
  ALTACORE_MAKE_DH(CodeLiteralNode);

  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
  }

  return info;
};
