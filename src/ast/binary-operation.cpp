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
  info->leftType = DET::Type::getUnderlyingType(info->left.get());
  info->rightType = DET::Type::getUnderlyingType(info->right.get());
  info->commonType = Shared::convertOperatorTypeRTC(info->type);

  if (info->leftType->klass && info->leftType->pointerLevel() < 1) {
    info->operatorMethod = info->leftType->klass->findOperator(info->commonType, Shared::ClassOperatorOrientation::Left, info->rightType);
  }

  if (info->operatorMethod == nullptr && info->rightType->klass && info->rightType->pointerLevel() < 1) {
    info->operatorMethod = info->rightType->klass->findOperator(info->commonType, Shared::ClassOperatorOrientation::Right, info->leftType);
  }

  if (info->operatorMethod == nullptr) {
    bool usedCast = false;

    // try to find a common type for both types
    // for now, just use a very algorithmically-dumb method
    // of finding the first castable type (first try left, then try right)
    //
    // in the future, the code should be moved to its own (probably static)
    // method in AltaCore::DET::Type

    // TODO: create some sort of framework for finding types that match a certain criteria,
    //       regardless of which side it's on. e.g:
    //           DET::Type::test(info->leftType, info->rightType, [&](auto a, auto b) {
    //             return a->pointerLevel() > 0 && b->pointerLevel() == 0 && b->isNative
    //           })
    //       and have it return a bool-convertible pair-like object with [a, b] being in the order
    //       that the types matched
    if ((size_t)info->type >= (size_t)OperatorType::LogicalAnd) {
      info->commonOperandType = std::make_shared<DET::Type>(DET::NativeType::Bool);
    } else if (
      (info->leftType->pointerLevel() > 0 && info->rightType->pointerLevel() == 0 && info->rightType->isNative) ||
      (info->rightType->pointerLevel() > 0 && info->leftType->pointerLevel() == 0 && info->leftType->isNative)
    ) {
      if (info->leftType->pointerLevel() > 0) {
        info->commonOperandType = info->leftType;
      } else {
        info->commonOperandType = info->rightType;
      }
    } else if (
      info->leftType->pointerLevel() == 0 &&
      info->rightType->pointerLevel() == 0 &&
      info->leftType->isNative &&
      info->rightType->isNative
    ) {
      using NT = DET::NativeType;
      using TMF = DET::TypeModifierFlag;
      int leftSize = 0;
      int rightSize = 0;
      NT leftType = info->leftType->nativeTypeName;
      NT rightType = info->rightType->nativeTypeName;
      for (auto& mod: info->leftType->modifiers) {
        if (mod & (uint8_t)TMF::Long) {
          ++leftSize;
        }
        if (mod & (uint8_t)TMF::Short) {
          --leftSize;
        }
      }
      for (auto& mod: info->rightType->modifiers) {
        if (mod & (uint8_t)TMF::Long) {
          ++rightSize;
        }
        if (mod & (uint8_t)TMF::Short) {
          --rightSize;
        }
      }
      if (leftType == NT::Double && rightType == NT::Double) {
        info->commonOperandType = leftSize > rightSize ? info->leftType : info->rightType;
      } else if (leftType == NT::Double) {
        info->commonOperandType = info->leftType;
      } else if (rightType == NT::Double) {
        info->commonOperandType = info->rightType;
      } else if (leftType == NT::Float && rightType == NT::Float) {
        info->commonOperandType = leftSize > rightSize ? info->leftType : info->rightType;
      } else if (leftType == NT::Float) {
        info->commonOperandType = info->leftType;
      } else if (rightType == NT::Float) {
        info->commonOperandType = info->rightType;
      } else if (leftType == NT::UserDefined && rightType == NT::UserDefined) {
        info->commonOperandType = leftSize > rightSize ? info->leftType : info->rightType;
      } else if (leftType == NT::UserDefined) {
        info->commonOperandType = info->leftType;
      } else if (rightType == NT::UserDefined) {
        info->commonOperandType = info->rightType;
      } else if (leftType == NT::Integer && rightType == NT::Integer) {
        info->commonOperandType = leftSize > rightSize ? info->leftType : info->rightType;
      } else if (leftType == NT::Integer) {
        info->commonOperandType = info->leftType;
      } else if (rightType == NT::Integer) {
        info->commonOperandType = info->rightType;
      } else if (leftType == NT::Byte && rightType == NT::Byte) {
        info->commonOperandType = leftSize > rightSize ? info->leftType : info->rightType;
      } else if (leftType == NT::Byte) {
        info->commonOperandType = info->leftType;
      } else if (rightType == NT::Byte) {
        info->commonOperandType = info->rightType;
      } else if (leftType == NT::Bool && rightType == NT::Bool) {
        info->commonOperandType = leftSize > rightSize ? info->leftType : info->rightType;
      } else if (leftType == NT::Bool) {
        info->commonOperandType = info->leftType;
      } else if (rightType == NT::Bool) {
        info->commonOperandType = info->rightType;
      } else {
        ALTACORE_DETAILING_ERROR("cannot perform operation on void expressions");
      }
    } else {
      usedCast = true;
      auto isLeftNative = info->leftType->pointerLevel() == 0 && info->leftType->isNative;
      auto isOneNative = isLeftNative || (info->rightType->pointerLevel() == 0 && info->rightType->isNative);
      auto rightToLeft = DET::Type::findCast(info->rightType, info->leftType);
      auto leftToRight = DET::Type::findCast(info->leftType, info->rightType);
      if (
        (!isOneNative || isLeftNative) &&
        rightToLeft.size() > 0 &&
        !(
          info->leftType->pointerLevel() > 0 &&
          (size_t)info->type >= (size_t)Shared::OperatorType::Addition &&
          (size_t)info->type <= (size_t)Shared::OperatorType::BitwiseXor
        )
      ) {
        info->commonOperandType = info->leftType;
      } else {
        if (
          (!isOneNative || !isLeftNative) &&
          leftToRight.size() > 0  &&
          !(
            info->rightType->pointerLevel() > 0 &&
            (size_t)info->type >= (size_t)Shared::OperatorType::Addition &&
            (size_t)info->type <= (size_t)Shared::OperatorType::BitwiseXor
          )
        ) {
          info->commonOperandType = info->rightType;
        } else {
          std::string msg = "binary operation performed on incompatible types\n";
          msg += "left operand type = " + info->leftType->toString() + '\n';
          msg += "right operand type = " + info->rightType->toString();
          ALTACORE_DETAILING_ERROR(msg);
        }
      }
    }
    info->commonOperandType = info->commonOperandType->destroyReferences();

    if (
      (
        (
          info->leftType->pointerLevel() > 0 &&
          info->rightType->pointerLevel() > 0
        ) ||
        (
          info->commonOperandType->pointerLevel() > 0 &&
          usedCast
        )
      ) &&
      (size_t)info->type >= (size_t)Shared::OperatorType::Addition &&
      (size_t)info->type <= (size_t)Shared::OperatorType::BitwiseXor
    ) {
      ALTACORE_DETAILING_ERROR("Invalid operation performed on pointers");
    }
  } else {
    info->inputScope->hoist(info->operatorMethod);
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(BinaryOperation) {
  ALTACORE_VS_S(BinaryOperation);
  if (!left) ALTACORE_VALIDATION_ERROR("Binary operation must contain a left-hand node");
  if (!right) ALTACORE_VALIDATION_ERROR("Binary operation must contain a right-hand node");
  left->validate(stack, info->left);
  right->validate(stack, info->right);

  if (!info->operatorMethod && info->commonOperandType->pointerLevel() == 0 && !info->commonOperandType->isNative && !(info->commonOperandType->isFunction && info->commonOperandType->isRawFunction)) {
    std::string msg = "Operation impossible with given operands (no operator method, operands are not pointers, not native, and not raw functions)\n";
    msg += "deduced common operand type = " + info->commonOperandType->toString();
    ALTACORE_VALIDATION_ERROR(msg);
  }

  ALTACORE_VS_E;
};
