#include "../../include/altacore/ast/class-read-accessor-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassReadAccessorDefinitionStatement::nodeType() {
  return NodeType::ClassReadAccessorDefinitionStatement;
};

AltaCore::AST::ClassReadAccessorDefinitionStatement::ClassReadAccessorDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
{};

void AltaCore::AST::ClassReadAccessorDefinitionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  type->detail(scope);
  $bodyScope = std::make_shared<DET::Scope>(scope);
  body->detail($bodyScope);

  $function = DET::Function::create(scope, name, {}, type->$type);
  $function->isAccessor = true;
};

ALTACORE_AST_VALIDATE_D(ClassReadAccessorDefinitionStatement) {
  ALTACORE_VS_S;
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for class read accessor definition statement");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for class read accessor definition statement");
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for class read accessor definition statement");
  type->validate(stack);
  body->validate(stack);
  ALTACORE_VS_E;
};
