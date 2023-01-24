#include "../../include/altacore/ast/conditional-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ConditionalExpression::nodeType() {
  return NodeType::ConditionalExpression;
};

AltaCore::AST::ConditionalExpression::ConditionalExpression(
  std::shared_ptr<ExpressionNode> _test,
  std::shared_ptr<ExpressionNode> _primary,
  std::shared_ptr<ExpressionNode> _secondary
):
  test(_test),
  primaryResult(_primary),
  secondaryResult(_secondary)
  {};

ALTACORE_AST_DETAIL_D(ConditionalExpression) {
  ALTACORE_MAKE_DH(ConditionalExpression);
  info->test = test->fullDetail(scope);
  info->primaryResult = primaryResult->fullDetail(scope);
  info->secondaryResult = secondaryResult->fullDetail(scope);

  auto primaryType = AltaCore::DET::Type::getUnderlyingType(info->primaryResult.get());
  auto secondaryType = AltaCore::DET::Type::getUnderlyingType(info->secondaryResult.get());

  // try to find a common type between the operands
  if (*primaryType == *secondaryType) {
    // perfect; this is the best case
    info->commonType = primaryType;
  } else if (*primaryType->destroyReferences() == *secondaryType->destroyReferences()) {
    info->commonType = primaryType->destroyReferences();
  } else if (primaryType->isNative && secondaryType->isNative && !primaryType->isFunction && !secondaryType->isFunction) {
    // both types are native; find the best (normally the biggest) type to cast to
    if (primaryType->isFloatingPoint() || secondaryType->isFloatingPoint()) {
      // floating point takes precedence
      auto maxBits = std::max(primaryType->nativeTypeBits(), secondaryType->nativeTypeBits());
      if (maxBits > 32) {
        info->commonType = std::make_shared<DET::Type>(DET::NativeType::Double);
      } else {
        info->commonType = std::make_shared<DET::Type>(DET::NativeType::Float);
      }
    } else {
      // otherwise (if neither is floating point), go by whichever is larger, defaulting to the primary type
      if (secondaryType->nativeTypeBits() > primaryType->nativeTypeBits()) {
        info->commonType = secondaryType->destroyReferences();
      } else {
        info->commonType = primaryType->destroyReferences();
      }
    }
  } else {
    // otherwise, try a dumb algorithm of "cast secondary to primary" and then "cast primary to secondary" if that fails
    if (DET::Type::findCast(secondaryType, primaryType, false).size() > 0) {
      info->commonType = primaryType;
    } else if (DET::Type::findCast(primaryType, secondaryType, false).size() > 0) {
      info->commonType = secondaryType;
    } else {
      ALTACORE_DETAILING_ERROR("Incompatible result types for conditional expression");
    }
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(ConditionalExpression) {
  ALTACORE_VS_S(ConditionalExpression);
  if (!test) ALTACORE_VALIDATION_ERROR("empty test for conditional expression");
  if (!primaryResult) ALTACORE_VALIDATION_ERROR("empty primary result for conditional expression");
  if (!secondaryResult) ALTACORE_VALIDATION_ERROR("empty secondary result for conditional expression");
  test->validate(stack, info->test);
  primaryResult->validate(stack, info->primaryResult);
  secondaryResult->validate(stack, info->secondaryResult);
  ALTACORE_VS_E;
};
