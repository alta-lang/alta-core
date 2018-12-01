#include "../../include/altacore/ast/attribute-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AttributeStatement::nodeType() {
  return NodeType::AttributeStatement;
};

AltaCore::AST::AttributeStatement::AttributeStatement(std::shared_ptr<AltaCore::AST::AttributeNode> _attribute):
  attribute(_attribute)
  {};

void AltaCore::AST::AttributeStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  attribute->detail(scope);
};
