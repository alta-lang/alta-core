#include "../../include/altacore/ast/pointer-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::PointerExpression::nodeType() {
  return NodeType::PointerExpression;
};

void AltaCore::AST::PointerExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);
};

ALTACORE_AST_VALIDATE_D(PointerExpression) {
  
};