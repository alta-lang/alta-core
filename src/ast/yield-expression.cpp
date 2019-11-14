#include "../../include/altacore/ast/yield-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::YieldExpression::nodeType() {
  return NodeType::YieldExpression;
};

ALTACORE_AST_DETAIL_D(YieldExpression) {
  ALTACORE_MAKE_DH(YieldExpression);

  info->target = target->fullDetail(info->inputScope);

  auto func = Util::getFunction(info->inputScope).lock();

  if (!func || !func->isGenerator) {
    ALTACORE_DETAILING_ERROR("`yield` can only be used inside generators");
  }

  info->generator = func;

  return info;
};

ALTACORE_AST_VALIDATE_D(YieldExpression) {
  ALTACORE_VS_S(YieldExpression);
  ALTACORE_VS_E;
};
