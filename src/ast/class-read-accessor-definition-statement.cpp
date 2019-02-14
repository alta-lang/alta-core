#include "../../include/altacore/ast/class-read-accessor-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassReadAccessorDefinitionStatement::nodeType() {
  return NodeType::ClassReadAccessorDefinitionStatement;
};

AltaCore::AST::ClassReadAccessorDefinitionStatement::ClassReadAccessorDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
{};

ALTACORE_AST_DETAIL_D(ClassReadAccessorDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassReadAccessorDefinitionStatement);
  info->type = type->fullDetail(scope);
  info->bodyScope = std::make_shared<DET::Scope>(scope);
  info->body = body->fullDetail(info->bodyScope);

  info->function = DET::Function::create(scope, name, {}, info->type->type);
  info->function->isAccessor = true;
  return info;
};

ALTACORE_AST_VALIDATE_D(ClassReadAccessorDefinitionStatement) {
  ALTACORE_VS_S(ClassReadAccessorDefinitionStatement);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for class read accessor definition statement");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for class read accessor definition statement");
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for class read accessor definition statement");
  type->validate(stack, info->type);
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
