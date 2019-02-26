#include "../../include/altacore/ast/instanceof-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::InstanceofExpression::nodeType() {
  return NodeType::InstanceofExpression;
};

AltaCore::AST::InstanceofExpression::InstanceofExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::shared_ptr<AltaCore::AST::Type> _type):
  target(_target),
  type(_type)
  {};

ALTACORE_AST_DETAIL_D(InstanceofExpression) {
  ALTACORE_MAKE_DH(InstanceofExpression);

  info->target = target->fullDetail(scope);
  info->type = type->fullDetail(scope);

  return info;
};

ALTACORE_AST_VALIDATE_D(InstanceofExpression) {
  ALTACORE_VS_S(InstanceofExpression);

  target->validate(stack, info->target);
  type->validate(stack, info->type);

  ALTACORE_VS_E;
};
