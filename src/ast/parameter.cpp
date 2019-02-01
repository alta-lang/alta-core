#include "../../include/altacore/ast/parameter.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Parameter::nodeType() {
  return NodeType::Parameter;
};

AltaCore::AST::Parameter::Parameter(std::string _name, std::shared_ptr<AltaCore::AST::Type> _type, bool _isVariable):
  name(_name),
  type(_type),
  isVariable(_isVariable)
  {};

void AltaCore::AST::Parameter::detail(std::shared_ptr<DET::Scope> scope, bool hoist) {
  for (auto& attr: attributes) {
    attr->target = shared_from_this();
    attr->detail(scope);
  }
  return type->detail(scope, hoist);
};

ALTACORE_AST_VALIDATE_D(Parameter) {
  ALTACORE_VS_S;
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for parameter");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for parameter");
  type->validate(stack);
  for (auto& attr: attributes) {
    if (!attr) ALTACORE_VALIDATION_ERROR("empty attribute for parameter");
    attr->validate(stack);
  }
  ALTACORE_VS_E;
};
