#include "../../include/altacore/ast/cast-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::CastExpression::nodeType() {
  return NodeType::CastExpression;
};

AltaCore::AST::CastExpression::CastExpression() {};

void AltaCore::AST::CastExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);
  type->detail(scope);
};

ALTACORE_AST_VALIDATE_D(CastExpression) {
  ALTACORE_VS_S;
  if (!target) ALTACORE_VALIDATION_ERROR("empty target for cast expression");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for cast expression");
  target->validate(stack);
  type->validate(stack);
  ALTACORE_VS_E;
};
