#include "../../include/altacore/ast/class-member-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassMemberDefinitionStatement::nodeType() {
  return NodeType::ClassMemberDefinitionStatement;
};

AltaCore::AST::ClassMemberDefinitionStatement::ClassMemberDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(ClassMemberDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassMemberDefinitionStatement);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(ClassMemberDefinitionStatement) {
  ALTACORE_VS_S(ClassMemberDefinitionStatement);
  if (!varDef) ALTACORE_VALIDATION_ERROR("empty variable definition for class member");
  varDef->validate(stack, info->varDef);
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(ClassMemberDefinitionStatement) {
  ALTACORE_CAST_DH(ClassMemberDefinitionStatement);

  if (varDef == nullptr) throw std::runtime_error("bad computer. bad.");

  if (!info->varDef) {
    info->varDef = varDef->fullDetail(info->inputScope, noBody);
    info->varDef->variable->visibility = visibilityModifier;
  }
  if (!info->varDef->initializationExpression) {
    info->varDef = varDef->fullDetail(info->varDef, noBody);
  }

  return info;
};
