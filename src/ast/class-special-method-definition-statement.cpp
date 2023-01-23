#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"
#include "../../include/altacore/util.hpp"
#include <sstream>
#include <crossguid/guid.hpp>

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

  if (type == SpecialClassMethod::Constructor && info->isCastConstructor && parameters.size() != 1) {
    ALTACORE_VALIDATION_ERROR("constructors that can be used for \"from\" casts must have a single parameter");
  }

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
  auto thisType = std::make_shared<DET::Type>(info->klass);

  if (!info->method) {
    if (type == SpecialClassMethod::Constructor) {
      std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> params;
      std::vector<size_t> optionalParameterIndexes;

      for (size_t i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        auto det = param->fullDetail(info->inputScope, false);
        info->parameters.push_back(det);
        Util::exportClassIfNecessary(info->inputScope, det->type->type);
        params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
        if (param->defaultValue)
          optionalParameterIndexes.push_back(i);
      }

      info->method = DET::Function::create(info->inputScope, "constructor", params, voidType);
      info->method->visibility = visibilityModifier;
      info->method->isConstructor = true;

      info->isCopyConstructor = info->method->parameterVariables.size() == 1 && *info->method->parameterVariables.front()->type->deconstify(true) == *std::make_shared<DET::Type>(info->klass)->reference();

      if (optionalParameterIndexes.size() > 0) {
        info->optionalVariantFunctions = FunctionDefinitionNode::expandOptionalVariants(optionalParameterIndexes, info->method, parameters, info->parameters);
      }
    } else if (type == SpecialClassMethod::Destructor) {
      info->method = DET::Function::create(info->inputScope, "destructor", {}, voidType);
      info->method->isDestructor = true;
      info->method->visibility = visibilityModifier;
    } else if (type == SpecialClassMethod::From) {
      info->specialType = specialType->fullDetail(info->inputScope);
      std::stringstream uuidStream;
      uuidStream << xg::newGuid();
      info->method = DET::Function::create(info->inputScope, "@from@", {
        { "$", info->specialType->type, false, uuidStream.str() },
      }, thisType);
      info->method->visibility = visibilityModifier;
    } else if (type == SpecialClassMethod::To) {
      info->specialType = specialType->fullDetail(info->inputScope);
      info->method = DET::Function::create(info->inputScope, "@to@", {}, info->specialType->type);
      info->method->visibility = visibilityModifier;
      info->method->isMethod = true;
    } else {
      ALTACORE_DETAILING_ERROR("impossible error encountered: special method type not recognized");
    }

    info->method->parentClassType = std::make_shared<DET::Type>(info->klass, std::vector<uint8_t> { (uint8_t)TypeModifierFlag::Reference });
  }

  if (!noBody && !info->body) {
    info->body = body->fullDetail(info->method->scope);
  }

  if (info->attributes.size() != attributes.size()) {
    for (auto& attr: attributes) {
      info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
    }
  }

  return info;
};
