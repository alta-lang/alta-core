#include "../../include/altacore/ast/unary-operation.hpp"

const AltaCore::AST::NodeType AltaCore::AST::UnaryOperation::nodeType() {
  return NodeType::UnaryOperation;
};

AltaCore::AST::UnaryOperation::UnaryOperation(
  UOperatorType _type,
  std::shared_ptr<ExpressionNode> _target
):
  type(_type),
  target(_target)
  {};

ALTACORE_AST_DETAIL_D(UnaryOperation) {
  ALTACORE_MAKE_DH(UnaryOperation);
  info->type = type;
  info->target = target->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(UnaryOperation) {
  ALTACORE_VS_S(UnaryOperation);
  if (!target) ALTACORE_VALIDATION_ERROR("Unary operation must contain a target node");
  target->validate(stack, info->target);
  ALTACORE_VS_E;
};
