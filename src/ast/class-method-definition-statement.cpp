#include "../../include/altacore/ast/class-method-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassMethodDefinitionStatement::nodeType() {
  return NodeType::ClassMethodDefinitionStatement;
};

AltaCore::AST::ClassMethodDefinitionStatement::ClassMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

void AltaCore::AST::ClassMethodDefinitionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {

};
