#include "../../include/altacore/ast/throw-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ThrowStatement::nodeType() {
  return NodeType::ThrowStatement;
};

ALTACORE_AST_DETAIL_D(ThrowStatement) {
  ALTACORE_MAKE_DH(ThrowStatement);
  info->expression = expression->fullDetail(info->inputScope);
  auto type = DET::Type::getUnderlyingType(info->expression.get());
  auto realType = type->copy()->deconstify();
  info->inputScope->addPossibleError(realType);
  return info;
};

ALTACORE_AST_VALIDATE_D(ThrowStatement) {
  ALTACORE_VS_S(ThrowStatement);
  expression->validate(stack, info->expression);
  ALTACORE_VS_E;
};
