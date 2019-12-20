#include "../../include/altacore/ast/expression-node.hpp"
#include "../../include/altacore/detail-handles.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ExpressionNode::nodeType() {
  return NodeType::ExpressionNode;
};

void AltaCore::AST::ExpressionNode::detailAttributes(std::shared_ptr<DH::ExpressionNode> info) {
  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
  }
};
