#include "../../include/altacore/ast/class-member-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassMemberDefinitionStatement::nodeType() {
  return NodeType::ClassMemberDefinitionStatement;
};

AltaCore::AST::ClassMemberDefinitionStatement::ClassMemberDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

void AltaCore::AST::ClassMemberDefinitionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  if (varDef == nullptr) throw std::runtime_error("bad computer. bad.");
  varDef->detail(scope);
  varDef->$variable->visibility = visibilityModifier;
};

ALTACORE_AST_VALIDATE_D(ClassMemberDefinitionStatement) {
  ALTACORE_VS_S;
  if (!varDef) ALTACORE_VALIDATION_ERROR("empty variable definition for class member");
  varDef->validate(stack);
  ALTACORE_VS_E;
};
