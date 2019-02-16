#include "../../include/altacore/ast/class-method-definition-statement.hpp"
#include "../../include/altacore/det/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassMethodDefinitionStatement::nodeType() {
  return NodeType::ClassMethodDefinitionStatement;
};

AltaCore::AST::ClassMethodDefinitionStatement::ClassMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(ClassMethodDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassMethodDefinitionStatement);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(ClassMethodDefinitionStatement) {
  ALTACORE_VS_S(ClassMethodDefinitionStatement);
  if (!funcDef) ALTACORE_VALIDATION_ERROR("empty function definition for class method");
  funcDef->validate(stack, info->funcDef);
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(ClassMethodDefinitionStatement) {
  ALTACORE_CAST_DH(ClassMethodDefinitionStatement);

  if (funcDef == nullptr) throw std::runtime_error("stop that");
  if (!info->funcDef) {
    info->funcDef = funcDef->fullDetail(info->inputScope, noBody);
    info->funcDef->function->visibility = visibilityModifier;
    info->funcDef->function->isMethod = true;
    auto klass = info->inputScope->parentClass.lock();
    info->funcDef->function->parentClassType = std::make_shared<DET::Type>(klass, std::vector<uint8_t> { (uint8_t)TypeModifierFlag::Reference });
  }
  if (!info->funcDef->body) {
    info->funcDef = funcDef->fullDetail(info->funcDef, noBody);
  }

  return info;
};
