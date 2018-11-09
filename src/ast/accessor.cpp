#include "../include/altacore/ast/accessor.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Accessor::nodeType() {
  return NodeType::Accessor;
};

AltaCore::AST::Accessor::Accessor(AltaCore::AST::ExpressionNode* _target, std::string _query):
  target(_target),
  query(_query)
  {};

void AltaCore::AST::Accessor::detail(AltaCore::DET::Scope* scope) {
  target->detail(scope);
  // TODO: item lookup in `scope`
};