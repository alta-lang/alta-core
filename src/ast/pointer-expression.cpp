#include "../../include/altacore/ast/pointer-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::PointerExpression::nodeType() {
  return NodeType::PointerExpression;
};

void AltaCore::AST::PointerExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);
};

ALTACORE_AST_VALIDATE_D(PointerExpression) {
  ALTACORE_VS_S;
  if (!target) throw ValidationError("empty target for pointer expression");
  target->validate(stack);
  ALTACORE_VS_E;
};