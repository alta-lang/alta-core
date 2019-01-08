#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassSpecialMethodDefinitionStatement::nodeType() {
  return NodeType::ClassSpecialMethodDefinitionStatement;
};

AltaCore::AST::ClassSpecialMethodDefinitionStatement::ClassSpecialMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier, AltaCore::AST::SpecialClassMethod _type):
  visibilityModifier(_visibilityModifier),
  type(_type)
  {};

void AltaCore::AST::ClassSpecialMethodDefinitionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {

};
