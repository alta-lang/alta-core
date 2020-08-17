#include "../../include/altacore/ast/yield-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::YieldExpression::nodeType() {
  return NodeType::YieldExpression;
};

ALTACORE_AST_DETAIL_D(YieldExpression) {
  ALTACORE_MAKE_DH(YieldExpression);

  if (target) {
    info->target = target->fullDetail(info->inputScope);
  }

  auto func = Util::getFunction(info->inputScope).lock();

  if (!func || !(func->isGenerator || func->isAsync)) {
    ALTACORE_DETAILING_ERROR("`yield` can only be used inside generators and coroutines");
  }

  if (func->isAsync && target) {
    ALTACORE_DETAILING_ERROR("`yield` cannot return values in coroutines");
  }

  if (!func->isAsync && !target) {
    ALTACORE_DETAILING_ERROR("`yield` must return values in generators");
  }

  info->generator = func;

  return info;
};

ALTACORE_AST_VALIDATE_D(YieldExpression) {
  ALTACORE_VS_S(YieldExpression);

  if (target) {
    target->validate(stack, info->target);
  }

  ALTACORE_VS_E;
};
