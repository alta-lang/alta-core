#include "../../include/altacore/ast/function-declaration-node.hpp"
#include <algorithm>
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::FunctionDeclarationNode::nodeType() {
  return NodeType::FunctionDeclarationNode;
};

AltaCore::AST::FunctionDeclarationNode::FunctionDeclarationNode(
  std::string _name,
  std::vector<std::shared_ptr<AltaCore::AST::Parameter>> _parameters,
  std::shared_ptr<AltaCore::AST::Type> _returnType,
  std::vector<std::string> _modifiers
):
  name(_name),
  parameters(_parameters),
  returnType(_returnType),
  modifiers(_modifiers)
  {};

void AltaCore::AST::FunctionDeclarationNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>>> params;

  for (auto& param: parameters) {
    param->detail(scope);
    params.push_back(std::make_tuple(param->name, param->type->$type));
  }

  returnType->detail(scope);

  $function = DET::Function::create(scope, name, params, returnType->$type);
  scope->items.push_back($function);

  $function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  $function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if ($function->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back($function);
    }
  }
};
