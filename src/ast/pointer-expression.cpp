#include "../../include/altacore/ast/pointer-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::PointerExpression::nodeType() {
  return NodeType::PointerExpression;
};

ALTACORE_AST_DETAIL_D(PointerExpression) {
  ALTACORE_MAKE_DH(PointerExpression);
  info->target = target->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(PointerExpression) {
  ALTACORE_VS_S(PointerExpression);
  if (!target) ALTACORE_VALIDATION_ERROR("empty target for pointer expression");
  target->validate(stack, info->target);
  ALTACORE_VS_E;
};
