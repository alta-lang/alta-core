#include "../../include/altacore/ast/cast-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::CastExpression::nodeType() {
  return NodeType::CastExpression;
};

AltaCore::AST::CastExpression::CastExpression() {};

ALTACORE_AST_DETAIL_D(CastExpression) {
  ALTACORE_MAKE_DH(CastExpression);
  info->target = target->fullDetail(scope);
  info->type = type->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(CastExpression) {
  ALTACORE_VS_S(CastExpression);
  if (!target) ALTACORE_VALIDATION_ERROR("empty target for cast expression");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for cast expression");
  target->validate(stack, info->target);
  type->validate(stack, info->type);
  ALTACORE_VS_E;
};
