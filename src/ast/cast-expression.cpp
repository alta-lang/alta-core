#include "../../include/altacore/ast/cast-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::CastExpression::nodeType() {
  return NodeType::CastExpression;
};

AltaCore::AST::CastExpression::CastExpression() {};

ALTACORE_AST_DETAIL_D(CastExpression) {
  ALTACORE_MAKE_DH(CastExpression);
  info->target = target->fullDetail(scope);
  info->type = type->fullDetail(scope);

  info->targetType = DET::Type::getUnderlyingType(info->target.get());

  info->castPath = DET::Type::findCast(info->targetType, info->type->type, true);

  if (info->castPath.size() == 0) {
    ALTACORE_DETAILING_ERROR("no way to cast from (" + info->targetType->toString() + ") to (" + info->type->type->toString() + ')');
  }

  for (auto& component: info->castPath) {
    if (component.type == DET::CastComponentType::From || component.type == DET::CastComponentType::To) {
      info->usesFromOrTo = true;
      break;
    }
  }

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
