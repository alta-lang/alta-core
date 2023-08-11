#include "../../include/altacore/ast/dereference-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::DereferenceExpression::nodeType() {
  return NodeType::DereferenceExpression;
};

ALTACORE_AST_DETAIL_D(DereferenceExpression) {
  ALTACORE_MAKE_DH(DereferenceExpression);
  info->target = target->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(DereferenceExpression) {
  ALTACORE_VS_S(DereferenceExpression);
  if (!target) ALTACORE_VALIDATION_ERROR("empty target for dereference expression");
  target->validate(stack, info->target);

  auto targetType = DET::Type::getUnderlyingType(info->target.get());
  if (targetType->pointerLevel() == 0 && !targetType->isOptional) {
    ALTACORE_VALIDATION_ERROR("invalid dereference (not a pointer or optional)");
  }

  ALTACORE_VS_E;
};
