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

ALTACORE_AST_DETAIL_D(FunctionDeclarationNode) {
  ALTACORE_MAKE_DH(FunctionDeclarationNode);
  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;
  std::vector<std::shared_ptr<DET::Type>> publicFunctionalTypes;

  for (auto& param: parameters) {
    auto det = param->fullDetail(scope, false);
    if (det->type->type->isFunction) {
      publicFunctionalTypes.push_back(det->type->type);
    }
    info->parameters.push_back(det);
    params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
  }

  info->returnType = returnType->fullDetail(scope, false);
  if (info->returnType->type->isFunction) {
    publicFunctionalTypes.push_back(info->returnType->type);
  }

  info->function = DET::Function::create(scope, name, params, info->returnType->type);
  scope->items.push_back(info->function);

  for (auto& type: publicFunctionalTypes) {
    info->function->publicHoistedFunctionalTypes.push_back(type);
  }

  info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if (info->function->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->function);
    }
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(FunctionDeclarationNode) {
  ALTACORE_VS_S(FunctionDeclarationNode);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for function declaration");
  for (size_t i = 0; i < parameters.size(); i++) {
    auto& param = parameters[i];
    auto& paramDet = info->parameters[i];
    if (!param) ALTACORE_VALIDATION_ERROR("empty parameter for function declaration");
    param->validate(stack, paramDet);
  }
  if (!returnType) ALTACORE_VALIDATION_ERROR("empty return type for function declaration");
  returnType->validate(stack, info->returnType);
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifier for function declaration");
  }
  ALTACORE_VS_E;
};
