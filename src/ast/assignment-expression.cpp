#include "../../include/altacore/ast/assignment-expression.hpp"
#include "../../include/altacore/det/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AssignmentExpression::nodeType() {
  return NodeType::AssignmentExpression;
};

AltaCore::AST::AssignmentExpression::AssignmentExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::shared_ptr<AltaCore::AST::ExpressionNode> _value):
  target(_target),
  value(_value)
  {};

ALTACORE_AST_DETAIL_D(AssignmentExpression) {
  ALTACORE_MAKE_DH(AssignmentExpression);

  info->target = target->fullDetail(scope);
  info->value = value->fullDetail(scope);
  info->type = type;

  info->targetType = DET::Type::getUnderlyingType(info->target.get());
  info->valueType = DET::Type::getUnderlyingType(info->value.get());

  info->commonType = Shared::convertOperatorTypeRTC(info->type);

  if (info->targetType->klass && info->targetType->pointerLevel() < 1) {
    info->operatorMethod = info->targetType->klass->findOperator(info->commonType, Shared::ClassOperatorOrientation::Left, info->valueType);
  }

  if (info->operatorMethod) {
    info->inputScope->hoist(info->operatorMethod);
  }

  detailAttributes(info);

  return info;
};

ALTACORE_AST_VALIDATE_D(AssignmentExpression) {
  ALTACORE_VS_S(AssignmentExpression);
  target->validate(stack, info->target);
  value->validate(stack, info->value);

  if (!info->operatorMethod) {
    if (info->targetType->modifiers.size() > 0 && info->targetType->modifiers.front() & (uint8_t)Shared::TypeModifierFlag::Constant) {
      ALTACORE_VALIDATION_ERROR("can't assign to a constant");
    }

    if (!info->targetType->isCompatibleWith(*info->valueType)) {
      ALTACORE_VALIDATION_ERROR("source type is not compatible with the destination type for assignment expression");
    }

    if (type != AssignmentType::Simple && !info->targetType->isNative) {
      ALTACORE_VALIDATION_ERROR("cannot perform compound addition on non-native types");
    }
  }

  ALTACORE_VS_E;
};
