#include "../include/altacore/ast/function-definition-node.hpp"
#include <algorithm>

const AltaCore::AST::NodeType AltaCore::AST::FunctionDefinitionNode::nodeType() {
  return NodeType::FunctionDefinitionNode;
};

AltaCore::AST::FunctionDefinitionNode::FunctionDefinitionNode(
  std::string _name,
  std::vector<AltaCore::AST::Parameter*> _parameters,
  AltaCore::AST::Type* _returnType,
  std::vector<std::string> _modifiers,
  AltaCore::AST::BlockNode* _body
):
  name(_name),
  parameters(_parameters),
  returnType(_returnType),
  modifiers(_modifiers),
  body(_body)
  {};

void AltaCore::AST::FunctionDefinitionNode::detail(AltaCore::DET::Scope* scope) {
  std::vector<std::tuple<std::string, DET::Type*>> params;

  for (auto& param: parameters) {
    param->detail(scope);
    params.push_back(std::make_tuple(param->name, param->type->$type));
  }

  returnType->detail(scope);

  $function = new DET::Function(scope, name, params, returnType->$type);
  scope->items.push_back($function);

  $function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();

  for (auto& stmt: body->statements) {
    stmt->detail($function->scope);
  }
};