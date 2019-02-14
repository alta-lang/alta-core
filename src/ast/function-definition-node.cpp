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

ALTACORE_AST_DETAIL_D(FunctionDefinitionNode) {
  ALTACORE_MAKE_DH(FunctionDefinitionNode);
  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

  for (auto& param: parameters) {
    auto det = param->fullDetail(scope, false);
    info->parameters.push_back(det);
    params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
  }

  info->returnType = returnType->fullDetail(scope, false);

  info->function = DET::Function::create(scope, name, params, info->returnType->type);
  scope->items.push_back(info->function);
  
  info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  info->body = body->fullDetail(info->function->scope);

  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(scope, shared_from_this(), info));
  }

  if (info->function->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->function);
    }
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(FunctionDefinitionNode) {
  ALTACORE_VS_S(FunctionDefinitionNode);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for function definition");
  for (size_t i = 0; i < attributes.size(); i++) {
    auto& attr = attributes[i];
    auto& attrDet = info->attributes[i];
    if (!attr) ALTACORE_VALIDATION_ERROR("empty attribute for parameter");
    attr->validate(stack, attrDet);
  }
  for (size_t i = 0; i < parameters.size(); i++) {
    auto& param = parameters[i];
    auto& paramDet = info->parameters[i];
    if (!param) ALTACORE_VALIDATION_ERROR("empty parameter for function definition");
    param->validate(stack, paramDet);
  }
  if (!returnType) ALTACORE_VALIDATION_ERROR("empty return type for function definition");
  returnType->validate(stack, info->returnType);
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifier for function definition");
  }
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for function definition");
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
