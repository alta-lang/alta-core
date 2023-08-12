#include "../../include/altacore/ast/function-call-expression.hpp"
#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/simple-map.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::FunctionCallExpression::nodeType() {
  return NodeType::FunctionCallExpression;
};

AltaCore::AST::FunctionCallExpression::FunctionCallExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::vector< std::pair<std::string, std::shared_ptr<AltaCore::AST::ExpressionNode>>> _arguments):
  target(_target),
  arguments(_arguments)
  {};

std::tuple<size_t, ALTACORE_MAP<size_t, size_t>, std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<AltaCore::AST::ExpressionNode>, std::shared_ptr<AltaCore::DH::ExpressionNode>>, std::vector<std::pair<std::shared_ptr<AltaCore::AST::ExpressionNode>, std::shared_ptr<AltaCore::DH::ExpressionNode>>>>>> AltaCore::AST::FunctionCallExpression::findCompatibleCall(std::vector<std::tuple<std::string, std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>> arguments, std::vector<std::shared_ptr<DET::Type>> targetTypes) {
  if (targetTypes.size() < 1) {
    throw std::runtime_error("Can't call an unknown expression like a function");
  }

  bool found = false; // whether we found the right function
  bool possible = false; // whether we found any function at all
  std::vector<std::tuple<size_t, std::vector<size_t>, std::shared_ptr<DET::Type>, std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>>, ALTACORE_MAP<size_t, size_t>>> compatibles;
  for (size_t index = 0; index < targetTypes.size(); index++) {
    auto& targetType = targetTypes[index];
    if (!targetType) continue;
    if (!targetType->isFunction) continue;
    possible = true;
    //if (targetType->parameters.size() != arguments.size()) continue;
    ALTACORE_MAP<size_t, std::pair<std::string, ALTACORE_VARIANT<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>>> argumentsInOrder;
    std::vector<size_t> compatiblities(targetType->parameters.size(), 0);
    size_t funcArgIndex = 0;
    bool ok = arguments.size() >= targetType->requiredArgumentCount();
    ALTACORE_MAP<size_t, size_t> argMap;
    for (size_t i = 0; i < targetType->parameters.size(); i++) {
      if (std::get<2>(targetType->parameters[i])) {
        argumentsInOrder[i] = std::make_pair(std::get<0>(targetType->parameters[i]), std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>());
      }
    }
    for (size_t i = 0; i < arguments.size(); i++) {
      if (!ok) break;
      auto& [argName, argExpr, argDet] = arguments[i];
      auto types = DET::Type::getUnderlyingTypes(argDet.get());
      if (argName.empty()) {
        if (funcArgIndex >= targetType->parameters.size()) {
          ok = false;
          break;
        }
      } else {
        bool found = false;
        for (size_t j = 0; j < targetType->parameters.size(); j++) {
          if (std::get<0>(targetType->parameters[j]) == argName) {
            funcArgIndex = j;
            found = true;
            break;
          }
        }
        if (!found) {
          ok = false;
          break;
        }
      }
      size_t compatiblity = 0;
      std::shared_ptr<DET::Type> finalType = nullptr;
      for (auto& maybeType: types) {
        auto type = maybeType->isAccessor ? maybeType->returnType : maybeType;
        auto currentCompat = std::get<1>(targetType->parameters[funcArgIndex])->compatiblity(*type);
        if (currentCompat > 0) {
          if (AltaCore::DET::Type::findCast(type, std::get<1>(targetType->parameters[funcArgIndex])).size() == 0) {
            currentCompat = 0;
          }
        }
        if (currentCompat > compatiblity) {
          compatiblity = currentCompat;
          finalType = type;
          if (argExpr->nodeType() == NodeType::Fetch) {
            auto fetch = std::dynamic_pointer_cast<AST::Fetch>(argExpr);
            auto fetchDet = std::dynamic_pointer_cast<DH::Fetch>(argDet);
            fetch->narrowTo(fetchDet, type);
          } else if (argExpr->nodeType() == NodeType::Accessor) {
            auto acc = std::dynamic_pointer_cast<AST::Accessor>(argExpr);
            auto accDet = std::dynamic_pointer_cast<DH::Accessor>(argDet);
            acc->narrowTo(accDet, type);
          }
        }
      }
      if (compatiblity == 0) {
        if (std::get<2>(targetType->parameters[funcArgIndex]) && funcArgIndex + 1 < targetType->parameters.size()) {
          funcArgIndex++;
          for (auto& maybeType: types) {
            auto type = maybeType->isAccessor ? maybeType->returnType : maybeType;
            auto currentCompat = std::get<1>(targetType->parameters[funcArgIndex])->compatiblity(*type);
            if (currentCompat > compatiblity) {
              compatiblity = currentCompat;
              finalType = type;
              if (argExpr->nodeType() == NodeType::Fetch) {
                auto fetch = std::dynamic_pointer_cast<AST::Fetch>(argExpr);
                auto fetchDet = std::dynamic_pointer_cast<DH::Fetch>(argDet);
                fetch->narrowTo(fetchDet, type);
              } else if (argExpr->nodeType() == NodeType::Accessor) {
                auto acc = std::dynamic_pointer_cast<AST::Accessor>(argExpr);
                auto accDet = std::dynamic_pointer_cast<DH::Accessor>(argDet);
                acc->narrowTo(accDet, type);
              }
            }
          }
          if (compatiblity == 0) {
            ok = false;
            break;
          }
        } else {
          ok = false;
          break;
        }
      }
      if (compatiblities[funcArgIndex] == 0) {
        compatiblities[funcArgIndex] = compatiblity;
      } else if (compatiblity < compatiblities[funcArgIndex]) {
        compatiblities[funcArgIndex] = compatiblity;
      }
      if (std::get<2>(targetType->parameters[funcArgIndex])) {
        if (argumentsInOrder.find(funcArgIndex) == argumentsInOrder.end()) {
          argumentsInOrder[funcArgIndex] = { argName, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>() };
        }
        ALTACORE_VARIANT_GET<std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>(argumentsInOrder[funcArgIndex].second).emplace_back(argExpr, argDet);
      } else {
        argumentsInOrder[funcArgIndex] = std::make_pair(argName, std::make_pair(argExpr, argDet));
      }
      argMap[i] = funcArgIndex;
      if (!std::get<2>(targetType->parameters[funcArgIndex])) {
        funcArgIndex++;
      }
    }
    if (!ok) continue;
    found = true;
    std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>> args(argumentsInOrder.size(), std::make_pair(nullptr, nullptr));
    for (auto& [i, arg]: argumentsInOrder) {
      args[i] = arg.second;
    }
    compatibles.push_back({ index, compatiblities, targetType, args, argMap });
  }

  size_t mostCompatibleIndex = SIZE_MAX;
  std::vector<size_t> mostCompatibleCompatiblities;
  std::shared_ptr<DET::Type> mostCompatibleType = nullptr;
  std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>> mostCompatibleArguments;
  ALTACORE_MAP<size_t, size_t> mostCompatibleArgMap;
  for (auto& [index, compatabilities, type, args, argMap]: compatibles) {
    if (mostCompatibleIndex != SIZE_MAX) {
      size_t numberGreater = 0;
      // the required argument count check is there to prevent variable parameter functions from overriding
      // fixed parameter functions that are defined before them
      // (if only the required number of arguments are given, the function defined first should be given precedence)
      if (compatabilities.size() > mostCompatibleCompatiblities.size() && arguments.size() > mostCompatibleType->requiredArgumentCount()) {
        numberGreater = SIZE_MAX;
      } else if (compatabilities.size() >= mostCompatibleCompatiblities.size()) {
        // TODO: this doesn't handle the case where the variable parameters come in the middle of the parameter list
        for (size_t i = 0; i < mostCompatibleCompatiblities.size(); i++) {
          if (compatabilities[i] > mostCompatibleCompatiblities[i]) {
            numberGreater++;
          }
        }
      }
      if (numberGreater > 0) {
        mostCompatibleIndex = index;
        mostCompatibleType = type;
        mostCompatibleCompatiblities = compatabilities;
        mostCompatibleArguments = args;
        mostCompatibleArgMap = argMap;
      }
    } else {
      mostCompatibleIndex = index;
      mostCompatibleType = type;
      mostCompatibleCompatiblities = compatabilities;
      mostCompatibleArguments = args;
      mostCompatibleArgMap = argMap;
    }
  }

  return { mostCompatibleIndex, mostCompatibleArgMap, mostCompatibleArguments };
};

ALTACORE_AST_DETAIL_D(FunctionCallExpression) {
  ALTACORE_MAKE_DH(FunctionCallExpression);
  info->maybe = maybe;
  info->target = target->fullDetail(scope);

  std::vector<std::tuple<std::string, std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>> argsWithDet;

  for (auto& [name, arg]: arguments) {
    auto det = arg->fullDetail(scope);
    argsWithDet.emplace_back(name, arg, det);
    info->arguments.push_back(det);
  }

  auto targetTypes = DET::Type::getUnderlyingTypes(info->target.get());

  bool genericFunctionError = false;
  for (auto& type: targetTypes) {
    if (!type) {
      genericFunctionError = true;
    } else if (type->isAccessor) {
      type = type->returnType;
    }
  }

  size_t index;
  ALTACORE_MAP<size_t, size_t> argMap;
  using ArgumentDetails = std::pair<std::shared_ptr<AltaCore::AST::ExpressionNode>, std::shared_ptr<AltaCore::DH::ExpressionNode>>;
  std::vector<ALTACORE_VARIANT<ArgumentDetails, std::vector<ArgumentDetails>>> adjArgs;

  info->isSpecialScheduleMethod = false;
  if (auto acc = std::dynamic_pointer_cast<DH::Accessor>(info->target)) {
    if (acc->narrowedTo && acc->narrowedTo->name == "schedule") {
      if (auto pscope = acc->narrowedTo->parentScope.lock()) {
        if (auto mod = Util::getModule(pscope.get()).lock()) {
          if (mod->id == mod->internal.coroutinesModule->id) {
            info->isSpecialScheduleMethod = true;
          }
        }
      }
    }
  }

  if (info->isSpecialScheduleMethod) {
    if (arguments.size() != 1) {
      ALTACORE_DETAILING_ERROR("`schedule` requires a single argument");
    }

    auto argType = DET::Type::getUnderlyingType(info->arguments[0].get());
    if (!argType->klass || argType->klass->name != "@Coroutine@") {
      ALTACORE_DETAILING_ERROR("argument to `schedule` must be a coroutine state instance");
    }

    index = 0;
    argMap = ALTACORE_MAP<size_t, size_t> { {0, 0} };
    adjArgs = std::vector<ALTACORE_VARIANT<ArgumentDetails, std::vector<ArgumentDetails>>> { ArgumentDetails { arguments[0].second, info->arguments[0] } };
  } else {
    std::tie(index, argMap, adjArgs) = findCompatibleCall(argsWithDet, targetTypes);
  }

  if (index == SIZE_MAX && genericFunctionError) {
    ALTACORE_DETAILING_ERROR("target function not found (and was possibly generic and wasn't instantiated)");
  }

  if (index != SIZE_MAX) {
    auto mostCompatibleType = targetTypes[index];
    info->targetType = mostCompatibleType;
    std::shared_ptr<DET::Function> func = nullptr;
    if (target->nodeType() == NodeType::Fetch) {
      auto fetch = std::dynamic_pointer_cast<AST::Fetch>(target);
      auto fetchDet = std::dynamic_pointer_cast<DH::Fetch>(info->target);
      fetch->narrowTo(fetchDet, index);
      func = std::dynamic_pointer_cast<DET::Function>(fetchDet->narrowedTo);
    } else if (target->nodeType() == NodeType::Accessor) {
      auto acc = std::dynamic_pointer_cast<AST::Accessor>(target);
      auto accDet = std::dynamic_pointer_cast<DH::Accessor>(info->target);
      acc->narrowTo(accDet, index);
      func = std::dynamic_pointer_cast<DET::Function>(accDet->narrowedTo);
      //acc->narrowTo(accDet, mostCompatibleType);
    }
    if (info->targetType->isMethod) {
      info->isMethodCall = true;
      auto tgt = std::dynamic_pointer_cast<AST::Accessor>(target);
      auto tgtInfo = std::dynamic_pointer_cast<DH::Accessor>(info->target);

      if (!tgt || !tgtInfo) {
        ALTACORE_DETAILING_ERROR("Class methods must be accessed through `this`");
      }

      info->methodClassTarget = tgt->target;
      info->methodClassTargetInfo = tgtInfo->target;
    }
    if (func) {
      func->doneDetailing.listen([=]() {
        for (auto& type: func->scope->typesThrown) {
          info->inputScope->addPossibleError(type);
        }
      });
    }
  }

  if (index == SIZE_MAX) {
    std::string message = "no functions were found that match the call signature\n";
    message += "argument types are: (";
    bool isFirst = true;
    for (size_t i = 0; i < arguments.size(); i++) {
      if (isFirst) {
        isFirst = false;
      } else {
        message += ", ";
      }
      auto& [argName, argExpr, argDet] = argsWithDet[i];
      auto types = DET::Type::getUnderlyingTypes(argDet.get());
      bool isFirst2 = true;
      for (auto& type: types) {
        if (isFirst2) {
          isFirst2 = false;
        } else {
          message += " | ";
        }
        message += type->toString();
      }
    }
    message += ")\n";
    message += "available call signatures:\n";
    bool foundOne = false;
    for (size_t index = 0; index < targetTypes.size(); index++) {
      auto& targetType = targetTypes[index];
      if (!targetType) continue;
      if (!targetType->isFunction) continue;
      foundOne = true;
      message += "  (";
      bool isFirst = true;
      for (auto& [name, type, variadic, id]: targetType->parameters) {
        if (isFirst) {
          isFirst = false;
        } else {
          message += ", ";
        }
        if (!name.empty())
          message += name + ": ";
        message += type->toString();
      }
      message += ") => " + targetType->returnType->toString() + "\n";
    }
    if (!foundOne) {
      message += "  **none**";
    }
    ALTACORE_DETAILING_ERROR(message);
  }

  if (maybe) {
    info->inputScope->hoist(info->targetType->returnType->makeOptional());
  }

  info->adjustedArguments = adjArgs;

  info->argumentMap = argMap;

  return info;
};

ALTACORE_AST_VALIDATE_D(FunctionCallExpression) {
  ALTACORE_VS_S(FunctionCallExpression);
  if (!target) ALTACORE_VALIDATION_ERROR("empty target for function call");
  for (size_t i = 0; i < arguments.size(); i++) {
    auto& [name, arg] = arguments[i];
    auto& argDet = info->arguments[i];
    if (!arg) ALTACORE_VALIDATION_ERROR("empty argument for function call");
    arg->validate(stack, argDet);
  }
  if (info->isMethodCall && !info->methodClassTarget) {
    ALTACORE_VALIDATION_ERROR("improperly detailed method class target for function call");
  }
  for (auto& adjArg: info->adjustedArguments) {
    if (auto args = ALTACORE_VARIANT_GET_IF<std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>(&adjArg)) {
      for (auto& [arg, argDet]: *args) {
        if (!arg) ALTACORE_VALIDATION_ERROR("empty adjusted argument for function call");
      }
    } else if (auto _arg = ALTACORE_VARIANT_GET_IF<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>(&adjArg)) {
      auto& [arg, argDet] = *_arg;
      if (!arg) ALTACORE_VALIDATION_ERROR("empty adjusted argument for function call");
    } else {
      throw std::runtime_error("this is *litterally* impossible");
    }
  }
  ALTACORE_VS_E;
};
