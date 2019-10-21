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

  if (auto retr = std::dynamic_pointer_cast<DH::RetrievalNode>(info->target)) {
    if (retr->items.size() == 1) {
      if (auto var = std::dynamic_pointer_cast<DET::Variable>(retr->items.front())) {
        if (var->isVariable) {
          ALTACORE_DETAILING_ERROR("Can't perform `instanceof` directly on a variable parameter (hint: access one of it's indexes first)");
        }
      }
    }
  }

  info->type = type->fullDetail(scope);

  return info;
};

ALTACORE_AST_VALIDATE_D(InstanceofExpression) {
  ALTACORE_VS_S(InstanceofExpression);

  target->validate(stack, info->target);
  type->validate(stack, info->type);

  ALTACORE_VS_E;
};
