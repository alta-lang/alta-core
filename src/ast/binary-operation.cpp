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

void AltaCore::AST::BinaryOperation::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  left->detail(scope);
  right->detail(scope);
};
