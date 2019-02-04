#include "../../include/altacore/ast/function-definition-node.hpp"
#include <algorithm>
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::FunctionDefinitionNode::nodeType() {
  return NodeType::FunctionDefinitionNode;
};

AltaCore::AST::FunctionDefinitionNode::FunctionDefinitionNode(
  std::string _name,
  std::vector<std::shared_ptr<AltaCore::AST::Parameter>> _parameters,
  std::shared_ptr<AltaCore::AST::Type> _returnType,
  std::vector<std::string> _modifiers,
  std::shared_ptr<AltaCore::AST::BlockNode> _body
):
  name(_name),
  parameters(_parameters),
  returnType(_returnType),
  modifiers(_modifiers),
  body(_body)
  {};

void AltaCore::AST::FunctionDefinitionNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

  for (auto& param: parameters) {
    param->detail(scope, false);
    params.push_back(std::make_tuple(param->name, param->type->$type, param->isVariable, param->id));
  }

  returnType->detail(scope, false);

  $function = DET::Function::create(scope, name, params, returnType->$type);
  scope->items.push_back($function);
  
  $function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  $function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  for (auto& stmt: body->statements) {
    stmt->detail($function->scope);
  }

  for (auto& attr: attributes) {
    attr->target = shared_from_this();
    attr->detail(scope);
  }

  if ($function->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back($function);
    }
  }
};

ALTACORE_AST_VALIDATE_D(FunctionDefinitionNode) {
  ALTACORE_VS_S;
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for function definition");
  for (auto& attr: attributes) {
    if (!attr) ALTACORE_VALIDATION_ERROR("empty attribute for parameter");
    attr->validate(stack);
  }
  for (auto& param: parameters) {
    if (!param) ALTACORE_VALIDATION_ERROR("empty parameter for function definition");
    param->validate(stack);
  }
  if (!returnType) ALTACORE_VALIDATION_ERROR("empty return type for function definition");
  returnType->validate(stack);
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifier for function definition");
  }
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for function definition");
  body->validate(stack);
  ALTACORE_VS_E;
};
