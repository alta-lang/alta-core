#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassSpecialMethodDefinitionStatement::nodeType() {
  return NodeType::ClassSpecialMethodDefinitionStatement;
};

AltaCore::AST::ClassSpecialMethodDefinitionStatement::ClassSpecialMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier, AltaCore::AST::SpecialClassMethod _type):
  visibilityModifier(_visibilityModifier),
  type(_type)
  {};

ALTACORE_AST_DETAIL_D(ClassSpecialMethodDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassSpecialMethodDefinitionStatement);
  info->klass = scope->parentClass.lock();
  if (type == SpecialClassMethod::Constructor) {
    std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> params;

    for (auto& param: parameters) {
      auto det = param->fullDetail(scope, false);
      info->parameters.push_back(det);
      params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
    }

    info->method = DET::Function::create(scope, "constructor", params, std::make_shared<DET::Type>(DET::NativeType::Void));

    info->method->visibility = visibilityModifier;

    info->body = body->fullDetail(info->method->scope);
  } else {
    ALTACORE_DETAILING_ERROR("destructors aren't supported yet");
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(ClassSpecialMethodDefinitionStatement) {
  ALTACORE_VS_S(ClassSpecialMethodDefinitionStatement);
  for (size_t i = 0; i < parameters.size(); i++) {
    auto& param = parameters[i];
    auto& paramDet = info->parameters[i];
    if (!param) ALTACORE_VALIDATION_ERROR("empty parameter for special class method");
    param->validate(stack, paramDet);
  }
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for special class method");
  body->validate(stack, info->body);
  if (!info->klass) ALTACORE_VALIDATION_ERROR("weird failure: class is empty for special class method (but that should be impossible)");
  if (!info->method) ALTACORE_VALIDATION_ERROR("failed to properly detail function for special class method");
  ALTACORE_VS_E;
};
