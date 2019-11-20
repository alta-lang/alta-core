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
  auto validationLoop = [&](std::shared_ptr<DH::FunctionDefinitionNode> info) {
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
  };
  if (generics.size() > 0) {
    for (auto& generic: info->genericInstantiations) {
      validationLoop(generic);
    }
  } else {
    validationLoop(info);
  }
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(FunctionDefinitionNode) {
  ALTACORE_CAST_DH(FunctionDefinitionNode);

  if (generics.size() > 0) {
    if (!info->function) {
      info->function = DET::Function::create(info->inputScope, name, {}, nullptr);
      info->function->ast = shared_from_this();
      info->function->info = info;
      info->function->genericParameterCount = generics.size();
      info->inputScope->items.push_back(info->function);

      info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
      info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

      if (info->function->isExport) {
        if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
          mod->exports->items.push_back(info->function);
        }
      }
    }
  } else {
    if (!info->function) {
      info->function = DET::Function::create(info->inputScope, name, {}, nullptr);
      info->inputScope->items.push_back(info->function);

      info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
      info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

      if (info->function->isExport) {
        if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
          mod->exports->items.push_back(info->function);
        }
      }

      info->function->isGenerator = info->isGenerator = isGenerator;
    }

    std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

    if (info->parameters.size() != parameters.size()) {
      for (auto& param: parameters) {
        auto det = param->fullDetail(info->function->scope, false);
        info->parameters.push_back(det);
        Util::exportClassIfNecessary(info->function->scope, det->type->type);
        params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
      }
    } else {
      for (size_t i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        auto& det = info->parameters[i];
        params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
      }
    }

    if (generatorParameter && !info->generatorParameter) {
      info->generatorParameter = generatorParameter->fullDetail(info->function->scope, false);
    }

    if (!info->returnType) {
      info->returnType = returnType->fullDetail(info->function->scope, false);
      Util::exportClassIfNecessary(info->function->scope, info->returnType->type);
    }

    if (info->isGenerator && !info->generator) {
      info->generator = DET::Class::create("@Generator@", info->function->scope, {}, true);
      info->function->scope->items.push_back(info->generator);
      auto doneVar = std::make_shared<DET::Variable>("done", std::make_shared<DET::Type>(DET::NativeType::Bool), info->generator->scope);
      info->generator->scope->items.push_back(doneVar);
      auto nextFunc = DET::Function::create(info->generator->scope, "next", {}, info->returnType->type->makeOptional());
      info->generator->scope->items.push_back(nextFunc);
      if (info->generatorParameter) {
        std::shared_ptr<DET::Function> nextFuncWithArgs = DET::Function::create(info->generator->scope, "next", {
          {"input", info->generatorParameter->type, false, "not-so-random-uuid"},
        }, info->returnType->type->makeOptional());
        info->generator->scope->items.push_back(nextFuncWithArgs);
      }
    }

    if (!info->function->returnType) {
      info->function->recreate(params, info->isGenerator ? std::make_shared<DET::Type>(info->generator) : info->returnType->type);
      info->function->generatorParameterType = info->generatorParameter ? info->generatorParameter->type : nullptr;
      info->function->generatorReturnType = info->isGenerator ? info->returnType->type : nullptr;
    }

    if (info->attributes.size() != attributes.size()) {
      info->attributes = Attributes::detailAttributes(attributes, info->inputScope, shared_from_this(), info);
    }

    info->function->beganThrowing.listen([=]() {
      info->function->scope->isTry = true;
    });

    if (!info->body && !noBody) {
      info->body = body->fullDetail(info->function->scope);
      info->function->doneDetailing.dispatch();
    }
  }

  return info;
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::AST::FunctionDefinitionNode::instantiateGeneric(std::shared_ptr<DH::FunctionDefinitionNode> info, std::vector<std::shared_ptr<DET::Type>> genericArguments) {
  if (genericArguments.size() != generics.size()) {
    return nullptr;
  }

  for (auto genericInst: info->genericInstantiations) {
    bool ok = true;
    for (size_t i = 0; i < genericInst->genericDetails.size(); i++) {
      if (auto target = std::dynamic_pointer_cast<DET::Type>(genericInst->genericDetails[i]->alias->target)) {
        if (!target->isExactlyCompatibleWith(*genericArguments[i])) {
          ok = false;
          break;
        }
      } else {
        ok = false;
        break;
      }
    }

    if (!ok) {
      continue;
    }

    return genericInst->function;
  }

  auto inst = std::make_shared<DH::GenericFunctionInstantiationDefinitionNode>(info->inputScope);
  info->genericInstantiations.push_back(inst);

  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

  inst->function = DET::Function::create(inst->inputScope, name, {}, nullptr);
  inst->function->ast = shared_from_this();
  inst->function->info = info;
  inst->function->genericParameterCount = info->function->genericParameterCount;

  inst->function->genericArguments = genericArguments;
  
  inst->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  inst->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  auto thisMod = Util::getModule(info->inputScope.get()).lock();
  auto& gDepEntry = thisMod->genericDependencies[inst->function->id];

  for (size_t i = 0; i < generics.size(); i++) {
    auto& generic = generics[i];
    auto& genericArg = genericArguments[i];

    auto det = generic->fullDetail(inst->function->scope);
    det->alias->target = genericArg;
    inst->genericDetails.push_back(det);

    Util::exportClassIfNecessary(inst->function->scope, genericArg);

    if (genericArg->klass) {
      auto thatMod = Util::getModule(genericArg->klass->parentScope.lock().get()).lock();
      gDepEntry.push_back(thatMod);
    }
  }

  for (auto& param: parameters) {
    auto det = param->fullDetail(inst->function->scope, false);
    inst->parameters.push_back(det);
    Util::exportClassIfNecessary(inst->function->scope, det->type->type);
    params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
  }

  inst->returnType = returnType->fullDetail(inst->function->scope, false);
  Util::exportClassIfNecessary(inst->function->scope, inst->returnType->type);

  inst->function->recreate(params, inst->returnType->type);

  for (auto& attr: attributes) {
    inst->attributes.push_back(attr->fullDetail(inst->inputScope, shared_from_this(), inst));
  }

  inst->body = body->fullDetail(inst->function->scope);
  inst->function->doneDetailing.dispatch();

  return inst->function;
};
