#include "../../include/altacore/ast/class-instantiation-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassInstantiationExpression::nodeType() {
  return NodeType::ClassInstantiationExpression;
};

void AltaCore::AST::ClassInstantiationExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  
};
