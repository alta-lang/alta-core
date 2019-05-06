#include "../../include/altacore/ast/binary-operation.hpp"

const AltaCore::AST::NodeType AltaCore::AST::BinaryOperation::nodeType() {
  return NodeType::BinaryOperation;
};

AltaCore::AST::BinaryOperation::BinaryOperation(
  OperatorType _type,
  std::shared_ptr<ExpressionNode> _left,
  std::shared_ptr<ExpressionNode> _right
):
  type(_type),
  left(_left),
  right(_right)
  {};

ALTACORE_AST_DETAIL_D(BinaryOperation) {
  ALTACORE_MAKE_DH(BinaryOperation);
  info->left = left->fullDetail(scope);
  info->right = right->fullDetail(scope);
  info->type = type;
  return info;
};

ALTACORE_AST_VALIDATE_D(BinaryOperation) {
  ALTACORE_VS_S(BinaryOperation);
  if (!left) ALTACORE_VALIDATION_ERROR("Binary operation must contain a left-hand node");
  if (!right) ALTACORE_VALIDATION_ERROR("Binary operation must contain a right-hand node");
  left->validate(stack, info->left);
  right->validate(stack, info->right);
  ALTACORE_VS_E;
};
