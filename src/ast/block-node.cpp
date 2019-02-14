#include "../../include/altacore/ast/block-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::BlockNode::nodeType() {
  return NodeType::BlockNode;
};

AltaCore::AST::BlockNode::BlockNode() {};
AltaCore::AST::BlockNode::BlockNode(std::vector<std::shared_ptr<AltaCore::AST::StatementNode>> _statements):
  statements(_statements)
  {};

ALTACORE_AST_DETAIL_D(BlockNode) {
  ALTACORE_MAKE_DH(BlockNode);
  for (auto& stmt: statements) {
    info->statements.push_back(stmt->fullDetail(scope));
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(BlockNode) {
  ALTACORE_VS_S(BlockNode);
  for (size_t i = 0; i < statements.size(); i++) {
    auto& stmt = statements[i];
    auto& stmtDet = info->statements[i];
    if (!stmt || !stmtDet) ALTACORE_VALIDATION_ERROR("Empty statement node in block node");
    stmt->validate(stack, stmtDet);
  }
  ALTACORE_VS_E;
};
