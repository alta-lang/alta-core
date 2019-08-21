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

  if (info->leftType->klass) {
    size_t highestCompat = 0;
    size_t compatIdx = SIZE_MAX;
    for (size_t i = 0; i < info->leftType->klass->operators.size(); ++i) {
      auto& op = info->leftType->klass->operators[i];
      if (op->operatorType != info->commonType) continue;
      if (op->orientation != Shared::ClassOperatorOrientation::Left) continue;
      auto compat = op->parameterVariables.front()->type->compatiblity(*info->rightType);
      if (compat > highestCompat) {
        highestCompat = compat;
        compatIdx = i;
      }
    }
    if (highestCompat != 0) {
      info->operatorMethod = info->leftType->klass->operators[compatIdx];
    }
  }

  if (info->operatorMethod == nullptr && info->rightType->klass) {
    size_t highestCompat = 0;
    size_t compatIdx = SIZE_MAX;
    for (size_t i = 0; i < info->rightType->klass->operators.size(); ++i) {
      auto& op = info->rightType->klass->operators[i];
      if (op->operatorType != info->commonType) continue;
      if (op->orientation != Shared::ClassOperatorOrientation::Right) continue;
      auto compat = op->parameterVariables.front()->type->compatiblity(*info->leftType);
      if (compat > highestCompat) {
        highestCompat = compat;
        compatIdx = i;
      }
    }
    if (highestCompat != 0) {
      info->operatorMethod = info->rightType->klass->operators[compatIdx];
    }
  }

  if (info->operatorMethod == nullptr) {
    // try to find a common type for both types
    // for now, just use a very algorithmically-dumb method
    // of finding the first castable type (first try left, then try right)
    //
    // in the future, this should be modified to have specific checks
    // for widening operations (such as int + double = double) and the code
    // should be moved to its own (probably static) method in AltaCore::DET::Type

    // TODO: create some sort of framework for finding types that match a certain criteria,
    //       regardless of which side it's on. e.g:
    //           DET::Type::test(info->leftType, info->rightType, [&](auto a, auto b) {
    //             return a->pointerLevel() > 0 && b->pointerLevel() == 0 && b->isNative
    //           })
    //       and have it return a bool-convertible pair-like object with [a, b] being in the order
    //       that the types matched
    if (
      (info->leftType->pointerLevel() > 0 && info->rightType->pointerLevel() == 0 && info->rightType->isNative) ||
      (info->rightType->pointerLevel() > 0 && info->leftType->pointerLevel() == 0 && info->leftType->isNative)
    ) {
      if (info->leftType->pointerLevel() > 0) {
        info->commonOperandType = info->leftType;
      } else {
        info->commonOperandType = info->rightType;
      }
    } else {
      auto rightToLeft = DET::Type::findCast(info->rightType, info->leftType);
      if (rightToLeft.size() > 0) {
        info->commonOperandType = info->leftType;
      } else {
        auto leftToRight = DET::Type::findCast(info->leftType, info->rightType);
        if (leftToRight.size() > 0) {
          info->commonOperandType = info->rightType;
        } else {
          ALTACORE_DETAILING_ERROR("binary operation performed on incompatible types");
        }
      }
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
  ALTACORE_VS_E;
};
