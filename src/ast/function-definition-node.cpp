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

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(FunctionDefinitionNode) {
  ALTACORE_MAKE_DH(FunctionDefinitionNode);
  return detail(info, noBody);
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

ALTACORE_AST_INFO_DETAIL_D(FunctionDefinitionNode) {
  ALTACORE_CAST_DH(FunctionDefinitionNode);

  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;
  std::vector<std::shared_ptr<DET::Type>> publicFunctionalTypes;

  if (info->parameters.size() != parameters.size()) {
    for (auto& param: parameters) {
      auto det = param->fullDetail(info->inputScope, false);
      if (det->type->type->isFunction) {
        publicFunctionalTypes.push_back(det->type->type);
      }
      info->parameters.push_back(det);
      params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
    }
  } else {
    for (size_t i = 0; i < parameters.size(); i++) {
      auto& param = parameters[i];
      auto& det = info->parameters[i];
      params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
    }
  }

  if (!info->returnType) {
    info->returnType = returnType->fullDetail(info->inputScope, false);
    if (info->returnType->type->isFunction) {
      publicFunctionalTypes.push_back(info->returnType->type);
    }
  }

  if (!info->function) {
    info->function = DET::Function::create(info->inputScope, name, params, info->returnType->type);
    info->inputScope->items.push_back(info->function);
    
    info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
    info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

    if (info->function->isExport) {
      if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
        mod->exports->items.push_back(info->function);
      }
    }
  }


  if (info->attributes.size() != attributes.size()) {
    for (auto& attr: attributes) {
      info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
    }
  }

  if (!info->body && !noBody) {
    info->body = body->fullDetail(info->function->scope);
  }

  return info;
};
