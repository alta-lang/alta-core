#include "../../include/altacore/ast/subscript-expression.hpp"
#include "../../include/altacore/det/type.hpp"

const std::shared_ptr<AltaCore::DET::Type> AltaCore::AST::SubscriptExpression::sizeType = std::make_shared<DET::Type>(DET::NativeType::Integer, std::vector<uint8_t> {
  (uint8_t)Shared::TypeModifierFlag::Long,
  (uint8_t)Shared::TypeModifierFlag::Long,
});

const AltaCore::AST::NodeType AltaCore::AST::SubscriptExpression::nodeType() {
  return NodeType::SubscriptExpression;
};

AltaCore::AST::SubscriptExpression::SubscriptExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::shared_ptr<AltaCore::AST::ExpressionNode> _index):
  target(_target),
  index(_index)
  {};

ALTACORE_AST_DETAIL_D(SubscriptExpression) {
  ALTACORE_MAKE_DH(SubscriptExpression);

  info->target = target->fullDetail(scope);
  info->index = index->fullDetail(scope);

  return info;
};

ALTACORE_AST_VALIDATE_D(SubscriptExpression) {
  ALTACORE_VS_S(SubscriptExpression);
  target->validate(stack, info->target);
  index->validate(stack, info->index);

  auto targetType = DET::Type::getUnderlyingType(info->target.get());
  auto indexType = DET::Type::getUnderlyingType(info->index.get());
  
  if (targetType->pointerLevel() < 1) {
    ALTACORE_VALIDATION_ERROR("can't index a non-pointer (yet)");
  }

  if (!indexType->isCompatibleWith(*sizeType)) {
    ALTACORE_VALIDATION_ERROR("can't use a non-integral value as an index (yet)");
  }

  ALTACORE_VS_E;
};
