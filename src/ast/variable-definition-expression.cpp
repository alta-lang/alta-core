#include "../../include/altacore/ast/variable-definition-expression.hpp"
#include "../../include/altacore/util.hpp"

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
  $variable->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if ($variable->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back($variable);
    }
  }
};

ALTACORE_AST_VALIDATE_D(VariableDefinitionExpression) {
  ALTACORE_VS_S;
  if (name.empty()) throw ValidationError("empty name for variable definition");
  if (!type) throw ValidationError("empty type for variable definition");
  type->validate(stack);
  if (initializationExpression) {
    initializationExpression->validate(stack);
  }
  for (auto& mod: modifiers) {
    if (mod.empty()) throw ValidationError("empty modifer for variable definition");
  }
  ALTACORE_VS_E;
};