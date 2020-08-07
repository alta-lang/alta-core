#include "../../include/altacore/ast/lambda-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::LambdaExpression::nodeType() {
  return NodeType::LambdaExpression;
};

ALTACORE_AST_DETAIL_D(LambdaExpression) {
  ALTACORE_MAKE_DH(LambdaExpression);

  info->function = DET::Function::create(info->inputScope, "@lambda@", {}, nullptr);

  info->function->isLambda = true;

  info->function->isLiteral = false;
  info->function->isExport = false;

  info->function->isGenerator = info->isGenerator = isGenerator;
  info->function->isAsync = info->isAsync = isAsync;

  if (isGenerator && isAsync) {
    ALTACORE_DETAILING_ERROR("a lambda cannot be asynchronous and be a generator");
  }

  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

  for (auto& param: parameters) {
    auto det = param->fullDetail(info->function->scope, false);
    info->parameters.push_back(det);
    params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
  }

  if (generatorParameter && !info->generatorParameter) {
    info->generatorParameter = generatorParameter->fullDetail(info->function->scope, false);
  }

  info->returnType = returnType->fullDetail(info->function->scope, false);
  Util::exportClassIfNecessary(info->function->scope, info->returnType->type);

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

  if (info->isAsync && !info->coroutine) {
    info->coroutine = DET::Class::create("@Coroutine@", info->function->scope, {}, true);
    info->function->scope->items.push_back(info->coroutine);
    auto doneVar = std::make_shared<DET::Variable>("done", std::make_shared<DET::Type>(DET::NativeType::Bool), info->coroutine->scope);
    info->coroutine->scope->items.push_back(doneVar);
    auto valueAcc = DET::Function::create(info->coroutine->scope, "value", {}, info->returnType->type->makeOptional());
    valueAcc->isAccessor = true;
    info->coroutine->scope->items.push_back(valueAcc);
    auto nextFunc = DET::Function::create(info->coroutine->scope, "next", {}, std::make_shared<DET::Type>(DET::NativeType::Void));
    info->coroutine->scope->items.push_back(nextFunc);
  }

  info->function->recreate(params, info->isGenerator ? std::make_shared<DET::Type>(info->generator) : (info->isAsync ? std::make_shared<DET::Type>(info->coroutine) : info->returnType->type));
  info->function->generatorParameterType = info->generatorParameter ? info->generatorParameter->type : nullptr;
  info->function->generatorReturnType = info->isGenerator ? info->returnType->type : nullptr;
  info->function->coroutineReturnType = info->isAsync ? info->returnType->type : nullptr;

  detailAttributes(info);

  info->function->beganThrowing.listen([=]() {
    info->function->scope->isTry = true;
  });

  info->body = body->fullDetail(info->function->scope);

  info->toReference = info->function->referencedVariables;

  for (size_t i = 0; i < info->toReference.size(); i++) {
    bool remove = false;
    for (auto& copyVar: info->toCopy) {
      if (copyVar->id == info->toReference[i]->id) {
        remove = true;
        break;
      }
    }
    if (remove) {
      info->toReference.erase(info->toReference.begin() + i);
      --i;
    }
  }

  for (size_t i = 0; i < info->toCopy.size(); ++i) {
    bool found = false;
    for (auto& item: info->function->referencedVariables) {
      if (item->id == info->toCopy[i]->id) {
        found = true;
        break;
      }
    }
    if (!found) {
      info->toCopy.erase(info->toCopy.begin() + i);
      --i;
    }
  }

  info->function->doneDetailing.dispatch();

  return info;
};

ALTACORE_AST_VALIDATE_D(LambdaExpression) {
  ALTACORE_VS_S(LambdaExpression);

  for (size_t i = 0; i < parameters.size(); ++i) {
    parameters[i]->validate(stack, info->parameters[i]);
  }

  returnType->validate(stack, info->returnType);

  if (generatorParameter)
    generatorParameter->validate(stack, info->generatorParameter);

  body->validate(stack, info->body);

  ALTACORE_VS_E;
};
