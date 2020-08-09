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

  info->targetType = DET::Type::getUnderlyingType(info->target.get());
  info->indexType = DET::Type::getUnderlyingType(info->index.get());

  if (info->targetType == nullptr) {
    std::shared_ptr<DET::Namespace> enumeration = nullptr;
    if (auto fetch = std::dynamic_pointer_cast<DH::Fetch>(info->target)) {
      if (fetch->narrowedTo) {
        if (auto maybeEnum = std::dynamic_pointer_cast<DET::Namespace>(fetch->narrowedTo)) {
          if (maybeEnum->underlyingEnumerationType) {
            enumeration = maybeEnum;
          }
        }
      }
    } else if (auto acc = std::dynamic_pointer_cast<DH::Accessor>(info->target)) {
      if (acc->narrowedTo) {
        if (auto maybeEnum = std::dynamic_pointer_cast<DET::Namespace>(acc->narrowedTo)) {
          if (maybeEnum->underlyingEnumerationType) {
            enumeration = maybeEnum;
          }
        }
      }
    }
    if (!enumeration) {
      ALTACORE_DETAILING_ERROR("Subscript called on expression with no type (and it wasn't an enumeration)");
    }
    info->enumeration = enumeration;
  } else {
    if (info->targetType->klass && info->targetType->pointerLevel() < 1) {
      info->operatorMethod = info->targetType->klass->findOperator(Shared::ClassOperatorType::Index, Shared::ClassOperatorOrientation::Unary, info->indexType);
    }

    if (info->operatorMethod) {
      info->inputScope->hoist(info->operatorMethod);
    } else {
      if (info->targetType->pointerLevel() < 1) {
        ALTACORE_DETAILING_ERROR("Target is not a pointer (and has no subscript operator method for the given index type)");
      }
      if (!info->indexType->isNative && info->indexType->pointerLevel() < 1) {
        ALTACORE_DETAILING_ERROR("Index is not a native type or pointer (and the target has no subscript operator method for the given index type)");
      }
    }
  }

  detailAttributes(info);

  return info;
};

ALTACORE_AST_VALIDATE_D(SubscriptExpression) {
  ALTACORE_VS_S(SubscriptExpression);
  target->validate(stack, info->target);
  index->validate(stack, info->index);

  if (info->enumeration) {
    if (info->reverseLookup) {
      auto tgtType = info->enumeration->underlyingEnumerationType;

      if (!tgtType->isCompatibleWith(*info->indexType)) {
        ALTACORE_VALIDATION_ERROR("reverse enumeration lookup expects an index with a type compatible with the enumeration's underlying type\nunderlying enumeration type: " + tgtType->toString() + "\nindex type: " + info->indexType->toString());
      }

      if (tgtType->klass && tgtType->pointerLevel() < 1) {
        info->operatorMethod = tgtType->klass->findOperator(Shared::ClassOperatorType::Equality, Shared::ClassOperatorOrientation::Left, tgtType);

        if (!info->operatorMethod) {
          info->operatorMethod = tgtType->klass->findOperator(Shared::ClassOperatorType::Equality, Shared::ClassOperatorOrientation::Right, tgtType);
        }
      }
      
      if (!info->operatorMethod && tgtType->pointerLevel() < 1 && (!tgtType->isNative || !tgtType->isRawFunction)) {
        ALTACORE_VALIDATION_ERROR("reverse enumeration lookup requires the enumeration's underlying type to be comparable with itself for equality");
      }

      info->operatorMethod = nullptr;
    } else {
      if (!DET::Type(DET::NativeType::Byte, DET::Type::createModifierVector({ { TypeModifierFlag::Constant, TypeModifierFlag::Pointer }, { TypeModifierFlag::Constant } })).isCompatibleWith(*info->indexType)) {
        ALTACORE_VALIDATION_ERROR("enumeration lookup expects an index with a type compatible with `const ptr const byte`\nindex type: " + info->indexType->toString());
      }
    }
  } else if (info->reverseLookup) {
    ALTACORE_VALIDATION_ERROR("@reverse is only for enumeration lookups");
  } else if (!info->operatorMethod) {
    if (info->targetType->pointerLevel() < 1) {
      ALTACORE_VALIDATION_ERROR("can't index a non-pointer (yet)");
    }

    if (!info->indexType->isCompatibleWith(*sizeType)) {
      ALTACORE_VALIDATION_ERROR("can't use a non-integral value as an index for a native type");
    }
  }

  ALTACORE_VS_E;
};
