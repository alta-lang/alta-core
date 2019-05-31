#include "../../include/altacore/ast/function-call-expression.hpp"
#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/simple-map.hpp"

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
      for (auto& type: types) {
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
        if (std::get<2>(targetType->parameters[funcArgIndex]) && funcArgIndex + 1 < targetType->parameters.size()) {
          funcArgIndex++;
          for (auto& type: types) {
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
      if (compatabilities.size() > mostCompatibleCompatiblities.size()) {
        numberGreater = SIZE_MAX;
      } else {
        for (size_t i = 0; i < compatabilities.size(); i++) {
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
  info->target = target->fullDetail(scope);

  std::vector<std::tuple<std::string, std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>> argsWithDet;

  for (auto& [name, arg]: arguments) {
    auto det = arg->fullDetail(scope);
    argsWithDet.emplace_back(name, arg, det);
    info->arguments.push_back(det);
  }

  auto targetTypes = DET::Type::getUnderlyingTypes(info->target.get());

  for (auto& type: targetTypes) {
    if (type->isAccessor) {
      type = type->returnType;
    }
  }

  auto [index, argMap, adjArgs] = findCompatibleCall(argsWithDet, targetTypes);

  if (index != SIZE_MAX) {
    auto mostCompatibleType = targetTypes[index];
    info->targetType = mostCompatibleType;
    if (target->nodeType() == NodeType::Fetch) {
      auto fetch = std::dynamic_pointer_cast<AST::Fetch>(target);
      auto fetchDet = std::dynamic_pointer_cast<DH::Fetch>(info->target);
      fetchDet->narrowedTo = fetchDet->items[index];
      //fetch->narrowTo(fetchDet, mostCompatibleType);
    } else if (target->nodeType() == NodeType::Accessor) {
      auto acc = std::dynamic_pointer_cast<AST::Accessor>(target);
      auto accDet = std::dynamic_pointer_cast<DH::Accessor>(info->target);
      accDet->narrowedTo = accDet->items[index];
      //acc->narrowTo(accDet, mostCompatibleType);
    }
    if (info->targetType->isMethod) {
      info->isMethodCall = true;
      auto tgt = std::dynamic_pointer_cast<AST::Accessor>(target);
      auto tgtInfo = std::dynamic_pointer_cast<DH::Accessor>(info->target);
      info->methodClassTarget = tgt->target;
      info->methodClassTargetInfo = tgtInfo->target;
    }
  }

  if (index == SIZE_MAX) {
    ALTACORE_DETAILING_ERROR("no functions found in the given expression");
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
