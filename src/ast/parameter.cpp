#include "../../include/altacore/ast/parameter.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Parameter::nodeType() {
  return NodeType::Parameter;
};

AltaCore::AST::Parameter::Parameter(std::string _name, std::shared_ptr<AltaCore::AST::Type> _type, bool _isVariable):
  name(_name),
  type(_type),
  isVariable(_isVariable)
  {};

std::shared_ptr<AltaCore::DH::Node> AltaCore::AST::Parameter::detail(std::shared_ptr<DET::Scope> scope, bool hoist) {
  ALTACORE_MAKE_DH(Parameter);
  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(scope, shared_from_this(), info));
  }
  info->type = type->fullDetail(scope, hoist);
  return info;
};

ALTACORE_AST_VALIDATE_D(Parameter) {
  ALTACORE_VS_S(Parameter);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for parameter");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for parameter");
  type->validate(stack, info->type);
  for (size_t i = 0; i < attributes.size(); i++) {
    auto& attr = attributes[i];
    auto& attrDet = info->attributes[i];
    if (!attr) ALTACORE_VALIDATION_ERROR("empty attribute for parameter");
    attr->validate(stack, attrDet);
  }
  ALTACORE_VS_E;
};
