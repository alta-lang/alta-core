#include "../../include/altacore/ast/function-definition-node.hpp"
#include <algorithm>
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::FunctionDefinitionNode::nodeType() {
  return NodeType::FunctionDefinitionNode;
};

std::vector<std::pair<std::shared_ptr<AltaCore::DET::Function>, std::vector<bool>>> AltaCore::AST::FunctionDefinitionNode::expandOptionalVariants(std::vector<size_t> optionalParameterIndexes, std::shared_ptr<DET::Function> original, std::vector<std::shared_ptr<Parameter>> parameters, std::vector<std::shared_ptr<DH::Parameter>> paramInfos) {
  auto inputScope = original->parentScope.lock();

  std::vector<std::pair<std::shared_ptr<AltaCore::DET::Function>, std::vector<bool>>> optionalVariantFunctions;
  if (optionalParameterIndexes.size() > 0) {
    /**
     * calculate all the possible optional parameter combinations
     *
     * basic example of how this needs to work:
     *
     *     given the following optional parameters:
     *         a b c d e
     *     these are the possible combinations:
     *     ----
     *
     *     a b c d e
     *     a b c d
     *     a c d e
     *     a b d e
     *     a b c e
     *     a b c
     *     a c d
     *     a d e
     *     a b e
     *     a b
     *     a c
     *     a d
     *     a e
     *     a
     *
     *     b c d e
     *     b c d
     *     b d e
     *     b c e
     *     b c
     *     b d
     *     b e
     *     b
     *
     *     c d e
     *     c d
     *     c e
     *     c
     *
     *     d e
     *     d
     *
     *     e
     *
     *     <empty>
     *
     * essentially, the pattern here is:
     *   1. iterate over each index as a starting element
     *   2. loop to choose how many elements to keep
     *   3. iterate over each possible keep index start
     *     - the elements to keep MUST wrap around
     *     - for example: in the example above, if we're iterating on the first element (`a`)
     *       and we're told to keep 2 elements and we're currently on the last element (`e`),
     *       we have to wrap around and include the first (includable) element (`b`)
     */

    // first loop: variant parameter start
    // decides what the first optional parameter of this variant is
    for (size_t i = 0; i < optionalParameterIndexes.size(); ++i) {
      // second loop: kept parameter count
      // decides how many parameters need to be kept in this variant
      // (not including the start parameter)
      //
      // should be `optionalParameterIndexes.size() - i - 1` and `kept >= 0`, but we're using an unsigned type,
      // so we can't subtract from 0. remember to do `kept - 1` to get the correct value for the counter
      for (size_t kept = optionalParameterIndexes.size() - i; kept > 0; --kept) {
        // third loop: combinator
        // iterates over the remaining, non-skipped parameters to add them to the variant
        // the current counter (`j`) actually indicates an element to start the included elements from
        // this needs to wrap around as described before
        for (size_t j = i + 1; j < optionalParameterIndexes.size(); ++j) {
          std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> variantParams;
          std::vector<bool> optionalValueProvided;

          size_t paramAfterCount = optionalParameterIndexes.size() - 1 - j;
          size_t wrappedParamCount = (kept < 2 || (kept - 2) < paramAfterCount) ? 0 : (kept - 2) - paramAfterCount;

          // add regular parameters up to the current start parameter
          for (size_t k = 0; k < optionalParameterIndexes[i]; ++k) {
            auto& param = parameters[k];
            auto& det = paramInfos[k];
            if (param->defaultValue) {
              optionalValueProvided.push_back(false);
              continue;
            }
            variantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
          }

          // add the current start parameter
          auto thisOptParam = parameters[optionalParameterIndexes[i]];
          auto& thisOptDet = paramInfos[optionalParameterIndexes[i]];
          optionalValueProvided.push_back(true);
          variantParams.push_back(std::make_tuple(thisOptParam->name, thisOptDet->type->type, thisOptParam->isVariable, thisOptParam->id));

          // add wrapped parameters
          for (size_t k = 0; k < wrappedParamCount; ++k) {
            // add regular parameters in between wrapped parameters
            for (size_t l = optionalParameterIndexes[i + k]; l < optionalParameterIndexes[i + 1 + k]; ++l) {
              auto& param = parameters[l];
              auto& det = paramInfos[l];
              if (param->defaultValue) {
                optionalValueProvided.push_back(false);
                continue;
              }
              variantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
            }

            auto thisOptParam = parameters[optionalParameterIndexes[i + 1 + k]];
            auto& thisOptDet = paramInfos[optionalParameterIndexes[i + 1 + k]];
            optionalValueProvided.push_back(true);
            variantParams.push_back(std::make_tuple(thisOptParam->name, thisOptDet->type->type, thisOptParam->isVariable, thisOptParam->id));
          }

          // add regular parameters up to the kept parameters
          for (size_t k = optionalParameterIndexes[i + wrappedParamCount] + 1; k < optionalParameterIndexes[j]; ++k) {
            auto& param = parameters[k];
            auto& det = paramInfos[k];
            if (param->defaultValue) {
              optionalValueProvided.push_back(false);
              continue;
            }
            variantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
          }

          // add the kept parameters
          for (size_t k = 0; k < (kept - 1) - wrappedParamCount; ++k) {
            auto thisOptParam = parameters[optionalParameterIndexes[j + k]];
            auto& thisOptDet = paramInfos[optionalParameterIndexes[j + k]];
            optionalValueProvided.push_back(true);
            variantParams.push_back(std::make_tuple(thisOptParam->name, thisOptDet->type->type, thisOptParam->isVariable, thisOptParam->id));

            // add regular parameters after the current kept parameter
            for (size_t l = optionalParameterIndexes[j + k] + 1; (k + 1 == (kept - 1) - wrappedParamCount) ? l < parameters.size() : l < optionalParameterIndexes[j + 1 + k]; ++l) {
              auto& param = parameters[l];
              auto& det = paramInfos[l];
              if (param->defaultValue) {
                optionalValueProvided.push_back(false);
                continue;
              }
              variantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
            }
          }

          if (kept == 1) {
            // add the remaining regular parameters
            for (size_t l = optionalParameterIndexes[j]; l < parameters.size(); ++l) {
              auto& param = parameters[l];
              auto& det = paramInfos[l];
              if (param->defaultValue) {
                optionalValueProvided.push_back(false);
                continue;
              }
              variantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
            }
          }

          bool same = i == 0 && kept == optionalParameterIndexes.size();
          for (size_t k = 0; k < optionalVariantFunctions.size(); ++k) {
            auto& thisVariant = std::get<0>(optionalVariantFunctions[k]);
            if (thisVariant->parameters.size() != variantParams.size())
              continue;
            same = true;
            for (size_t l = 0; l < variantParams.size(); ++l) {
              auto& [thisName, thisType, thisIsVariable, thisId] = variantParams[l];
              auto& [thatName, thatType, thatIsVariable, thatId] = thisVariant->parameters[l];
              if (thisName != thatName) {
                same = false;
                break;
              }
              if (!(*thisType == *thatType)) {
                same = false;
                break;
              }
              if (thisIsVariable != thatIsVariable) {
                same = false;
                break;
              }
              if (thisId != thatId) {
                same = false;
                break;
              }
            }
            if (same) {
              break;
            }
          }

          if (!same) {
            auto func = DET::Function::create(inputScope, original->name, variantParams, original->returnType, original->position);
            optionalVariantFunctions.push_back(std::make_pair(func, optionalValueProvided));

            func->isLiteral = original->isLiteral;
            func->isExport = original->isExport;

            if (func->isExport) {
              if (auto mod = Util::getModule(inputScope.get()).lock()) {
                mod->exports->items.push_back(func);
              }
            }

            func->isGenerator = original->isGenerator;
            func->isAsync = original->isAsync;
            func->visibility = original->visibility;
          }
        }
      }

      // add the solo parameter variant
      // (the one where the only optional parameter is the current one we're iterating on)
      //
      // only do this when there's a single optional parameter, because when there's more,
      // the loops will automatically take care of it
      if (i + 1 == optionalParameterIndexes.size() && i > 0) {
        std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> soloVariantParams;
        std::vector<bool> optionalValueProvided;

        for (size_t j = 0; j < optionalParameterIndexes[i]; ++j) {
          auto& param = parameters[j];
          auto& det = paramInfos[j];
          if (param->defaultValue) {
            optionalValueProvided.push_back(false);
            continue;
          }
          soloVariantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
        }

        auto idx = soloVariantParams.size();
        optionalValueProvided.push_back(true);
        soloVariantParams.push_back(std::make_tuple(parameters[optionalParameterIndexes[i]]->name, paramInfos[optionalParameterIndexes[i]]->type->type, parameters[optionalParameterIndexes[i]]->isVariable, parameters[optionalParameterIndexes[i]]->id));

        for (size_t j = optionalParameterIndexes[i] + 1; j < parameters.size(); ++j) {
          auto& param = parameters[j];
          auto& det = paramInfos[j];
          if (param->defaultValue) {
            optionalValueProvided.push_back(false);
            continue;
          }
          soloVariantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
        }

        auto func = DET::Function::create(inputScope, original->name, soloVariantParams, original->returnType, original->position);
        optionalVariantFunctions.push_back(std::make_pair(func, optionalValueProvided));

        func->isLiteral = original->isLiteral;
        func->isExport = original->isExport;

        if (func->isExport) {
          if (auto mod = Util::getModule(inputScope.get()).lock()) {
            mod->exports->items.push_back(func);
          }
        }

        func->isGenerator = original->isGenerator;
        func->isAsync = original->isAsync;
        func->visibility = original->visibility;
      }
    }

    // add the empty parameter variant
    // (the one where there are *no* optional parameters present)
    std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> emptyVariantParams;

    for (size_t i = 0; i < parameters.size(); ++i) {
      auto& param = parameters[i];
      auto& det = paramInfos[i];
      if (param->defaultValue)
        continue;
      emptyVariantParams.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
    }

    auto func = DET::Function::create(inputScope, original->name, emptyVariantParams, original->returnType, original->position);
    optionalVariantFunctions.push_back(std::make_pair(func, std::vector<bool>(optionalParameterIndexes.size(), false)));

    func->isLiteral = original->isLiteral;
    func->isExport = original->isExport;

    if (func->isExport) {
      if (auto mod = Util::getModule(inputScope.get()).lock()) {
        mod->exports->items.push_back(func);
      }
    }

    func->isGenerator = original->isGenerator;
    func->isAsync = original->isAsync;
    func->visibility = original->visibility;
    func->isMethod = original->isMethod;
    func->parentClassType = original->parentClassType;
  }

  return optionalVariantFunctions;
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
      info->function = DET::Function::create(info->inputScope, name, {}, nullptr, position);
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
      info->function = DET::Function::create(info->inputScope, name, {}, nullptr, position);
      info->inputScope->items.push_back(info->function);

      info->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
      info->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

      if (info->function->isExport) {
        if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
          mod->exports->items.push_back(info->function);
        }
      }

      info->function->isGenerator = info->isGenerator = isGenerator;
      info->function->isAsync = info->isAsync = isAsync;

      if (isGenerator && isAsync) {
        ALTACORE_DETAILING_ERROR("a function cannot be asynchronous and be a generator");
      }
    }

    std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;
    std::vector<size_t> optionalParameterIndexes;

    if (info->parameters.size() != parameters.size()) {
      for (size_t i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        auto det = param->fullDetail(info->function->scope, false);
        info->parameters.push_back(det);
        Util::exportClassIfNecessary(info->function->scope, det->type->type);
        params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
        if (param->defaultValue)
          optionalParameterIndexes.push_back(i);
      }
    } else {
      for (size_t i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        auto& det = info->parameters[i];
        params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
        if (param->defaultValue)
          optionalParameterIndexes.push_back(i);
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
      info->generator = DET::Class::create("@Generator@", info->function->scope, position, {}, true);
      info->function->scope->items.push_back(info->generator);
      auto doneVar = std::make_shared<DET::Variable>("done", std::make_shared<DET::Type>(DET::NativeType::Bool), position, info->generator->scope);
      info->generator->scope->items.push_back(doneVar);
      auto nextFunc = DET::Function::create(info->generator->scope, "next", {}, info->returnType->type->makeOptional(), position);
      info->generator->scope->items.push_back(nextFunc);
      if (info->generatorParameter) {
        std::shared_ptr<DET::Function> nextFuncWithArgs = DET::Function::create(info->generator->scope, "next", {
          {"input", info->generatorParameter->type, false, "not-so-random-uuid"},
        }, info->returnType->type->makeOptional(), position);
        info->generator->scope->items.push_back(nextFuncWithArgs);
      }
    }

    if (info->isAsync && !info->coroutine) {
      info->coroutine = DET::Class::create("@Coroutine@", info->function->scope, position, {}, true);
      info->function->scope->items.push_back(info->coroutine);
      auto doneVar = std::make_shared<DET::Variable>("done", std::make_shared<DET::Type>(DET::NativeType::Bool), position, info->coroutine->scope);
      doneVar->isLiteral = true;
      info->coroutine->scope->items.push_back(doneVar);
      auto valueAcc = DET::Function::create(info->coroutine->scope, "value", {}, info->returnType->type->makeOptional(), position);
      valueAcc->isAccessor = true;
      info->coroutine->scope->items.push_back(valueAcc);
      auto nextFunc = DET::Function::create(info->coroutine->scope, "next", {}, std::make_shared<DET::Type>(DET::NativeType::Void), position);
      info->coroutine->scope->items.push_back(nextFunc);

      auto coroutineStruct = DET::Class::create("@UserAccessibleCoroutineStructure@", info->function->scope, position, {}, true);
      auto idVar = std::make_shared<DET::Variable>("id", std::make_shared<DET::Type>(DET::NativeType::UserDefined, std::vector<uint8_t> {}, "uint64_t"), position, coroutineStruct->scope);
      idVar->isLiteral = true;
      coroutineStruct->scope->items.push_back(idVar);
      auto coroutineVar = std::make_shared<DET::Variable>("$coroutine", std::make_shared<DET::Type>(coroutineStruct, DET::Type::createModifierVector({ { TypeModifierFlag::Reference } })), position, info->function->scope);
      info->function->scope->items.push_back(coroutineVar);
    }

    if (!info->function->returnType) {
      info->function->recreate(params, info->isGenerator ? std::make_shared<DET::Type>(info->generator) : (info->isAsync ? std::make_shared<DET::Type>(info->coroutine) : info->returnType->type));
      info->function->generatorParameterType = info->generatorParameter ? info->generatorParameter->type : nullptr;
      info->function->generatorReturnType = info->isGenerator ? info->returnType->type : nullptr;
      info->function->coroutineReturnType = info->isAsync ? info->returnType->type : nullptr;
    }

    if (optionalParameterIndexes.size() > 0 && info->optionalVariantFunctions.size() == 0) {
      info->optionalVariantFunctions = expandOptionalVariants(optionalParameterIndexes, info->function, parameters, info->parameters);
      for (auto& func: info->optionalVariantFunctions) {
        info->inputScope->items.push_back(func.first);
      }
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

  inst->function = DET::Function::create(inst->inputScope, name, {}, nullptr, position);
  inst->function->ast = shared_from_this();
  inst->function->info = info;
  inst->function->genericParameterCount = info->function->genericParameterCount;

  inst->function->genericArguments = genericArguments;
  
  inst->function->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  inst->function->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();
  inst->function->isMethod = info->function->isMethod;
  inst->function->isGenerator = info->function->isGenerator;
  inst->function->isAsync = info->function->isAsync;
  inst->function->parentClassType = info->function->parentClassType;

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
      thatMod->dependents.push_back(thisMod);
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
