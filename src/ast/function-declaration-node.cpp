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
  std::vector<std::shared_ptr<DET::Type>> publicTypes;

  for (auto& param: parameters) {
    auto det = param->fullDetail(scope, false);
    if (det->type->type->isFunction || det->type->type->isUnion() || det->type->type->isOptional) {
      publicTypes.push_back(det->type->type);
    }
    info->parameters.push_back(det);
    params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
  }

  info->returnType = returnType->fullDetail(scope, false);
  if (info->returnType->type->isFunction || info->returnType->type->isUnion()  || info->returnType->type->isOptional) {
    publicTypes.push_back(info->returnType->type);
  }

  info->function = DET::Function::create(scope, name, params, info->returnType->type, position);
  scope->items.push_back(info->function);

  for (auto& type: publicTypes) {
    info->function->publicHoistedItems.push_back(type);
  }

  info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if (info->function->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->function);
    }
  }

  if (info->attributes.size() != attributes.size()) {
    info->attributes = Attributes::detailAttributes(attributes, info->inputScope, shared_from_this(), info);
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
  for (size_t i = 0; i < attributes.size(); i++) {
    auto& attr = attributes[i];
    auto& attrDet = info->attributes[i];
    if (!attr) ALTACORE_VALIDATION_ERROR("empty attribute for parameter");
    attr->validate(stack, attrDet);
  }
  ALTACORE_VS_E;
};
