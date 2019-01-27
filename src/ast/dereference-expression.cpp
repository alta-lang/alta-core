#include "../../include/altacore/ast/dereference-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::DereferenceExpression::nodeType() {
  return NodeType::DereferenceExpression;
};

void AltaCore::AST::DereferenceExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);
};

ALTACORE_AST_VALIDATE_D(DereferenceExpression) {
  ALTACORE_VS_S;
  if (!target) throw ValidationError("empty target for dereference expression");
  target->validate(stack);
  ALTACORE_VS_E;
};