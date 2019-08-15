#include "../../include/altacore/ast/class-operator-definition-statement.hpp"
#include "../../include/altacore/det/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassOperatorDefinitionStatement::nodeType() {
  return NodeType::ClassOperatorDefinitionStatement;
};

AltaCore::AST::ClassOperatorDefinitionStatement::ClassOperatorDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(ClassOperatorDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassOperatorDefinitionStatement);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(ClassOperatorDefinitionStatement) {
  ALTACORE_VS_S(ClassOperatorDefinitionStatement);
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(ClassOperatorDefinitionStatement) {
  ALTACORE_CAST_DH(ClassOperatorDefinitionStatement);
  return info;
};
