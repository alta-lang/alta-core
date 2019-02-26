#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassSpecialMethodDefinitionStatement::nodeType() {
  return NodeType::ClassSpecialMethodDefinitionStatement;
};

AltaCore::AST::ClassSpecialMethodDefinitionStatement::ClassSpecialMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier, AltaCore::AST::SpecialClassMethod _type):
  visibilityModifier(_visibilityModifier),
  type(_type)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(ClassSpecialMethodDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassSpecialMethodDefinitionStatement);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(ClassSpecialMethodDefinitionStatement) {
  ALTACORE_VS_S(ClassSpecialMethodDefinitionStatement);

  if (type == SpecialClassMethod::Destructor && parameters.size() > 0) {
    ALTACORE_VALIDATION_ERROR("destructors can't have parameters");
  }

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

ALTACORE_AST_INFO_DETAIL_D(ClassSpecialMethodDefinitionStatement) {
  ALTACORE_CAST_DH(ClassSpecialMethodDefinitionStatement);
  if (!info->klass) {
    info->klass = info->inputScope->parentClass.lock();
  }

  auto voidType = std::make_shared<DET::Type>(DET::NativeType::Void);

  if (type == SpecialClassMethod::Constructor) {
    if (!info->method) {
      std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> params;

      for (auto& param: parameters) {
        auto det = param->fullDetail(info->inputScope, false);
        info->parameters.push_back(det);
        params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
      }

      info->method = DET::Function::create(info->inputScope, "constructor", params, voidType);
      info->method->visibility = visibilityModifier;
    }
    if (!noBody && !info->body) {
      info->body = body->fullDetail(info->method->scope);
    }
  } else {
    if (!info->method) {
      info->method = DET::Function::create(info->inputScope, "destructor", {}, voidType);
      info->method->visibility = visibilityModifier;
    }
    if (!noBody && !info->body) {
      info->body = body->fullDetail(info->method->scope);
    }
  }

  if (info->attributes.size() != attributes.size()) {
    for (auto& attr: attributes) {
      info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
    }
  }

  return info;
};
