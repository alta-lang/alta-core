#include "../../include/altacore/ast/variable-definition-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::VariableDefinitionExpression::nodeType() {
  return NodeType::VariableDefinitionExpression;
};

AltaCore::AST::VariableDefinitionExpression::VariableDefinitionExpression(
  std::string _name,
  std::shared_ptr<AltaCore::AST::Type> _type,
  std::shared_ptr<AltaCore::AST::ExpressionNode> _initializationExpression
):
  name(_name),
  type(_type),
  initializationExpression(_initializationExpression)
  {};

void AltaCore::AST::VariableDefinitionExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  type->detail(scope);

  $variable = std::make_shared<DET::Variable>(name, type->$type, scope);
  scope->items.push_back($variable);

  if (initializationExpression != nullptr) {
    initializationExpression->detail(scope);
  }

  $variable->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
};
