#include "../../include/altacore/ast/function-call-expression.hpp"
#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include <unordered_map>

const AltaCore::AST::NodeType AltaCore::AST::FunctionCallExpression::nodeType() {
  return NodeType::FunctionCallExpression;
};

AltaCore::AST::FunctionCallExpression::FunctionCallExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::vector< std::pair<std::string, std::shared_ptr<AltaCore::AST::ExpressionNode>>> _arguments):
  target(_target),
  arguments(_arguments)
  {};

void AltaCore::AST::FunctionCallExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);

  for (auto& [name, arg]: arguments) {
    arg->detail(scope);
  }

  auto targetTypes = DET::Type::getUnderlyingTypes(target.get());
  if (targetTypes.size() < 1) {
    throw std::runtime_error("Can't call an unknown expression like a function");
  }

  bool found = false; // whether we found the right function
  bool possible = false; // whether we found any function at all
  std::vector<std::tuple<std::vector<size_t>, std::shared_ptr<DET::Type>, std::vector<ALTACORE_VARIANT<std::shared_ptr<ExpressionNode>, std::vector<std::shared_ptr<ExpressionNode>>>>, std::unordered_map<size_t, size_t>>> compatibles;
  for (auto& targetType: targetTypes) {
    if (!targetType->isFunction) continue;
    possible = true;
    //if (targetType->parameters.size() != arguments.size()) continue;
    std::unordered_map<size_t, std::pair<std::string, ALTACORE_VARIANT<std::shared_ptr<ExpressionNode>, std::vector<std::shared_ptr<ExpressionNode>>>>> argumentsInOrder;
    std::vector<size_t> compatiblities(targetType->parameters.size(), 0);
    size_t funcArgIndex = 0;
    bool ok = true;
    std::unordered_map<size_t, size_t> argMap;
    for (size_t i = 0; i < arguments.size(); i++) {
      auto& argument = arguments[i];
      auto types = DET::Type::getUnderlyingTypes(argument.second.get());
      if (argument.first.empty()) {
        if (funcArgIndex >= targetType->parameters.size()) {
          ok = false;
          break;
        }
      } else {
        bool found = false;
        for (size_t j = 0; j < targetType->parameters.size(); j++) {
          if (std::get<0>(targetType->parameters[j]) == argument.first) {
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
          if (argument.second->nodeType() == NodeType::Fetch) {
            auto fetch = std::dynamic_pointer_cast<AST::Fetch>(argument.second);
            fetch->narrowTo(type);
          } else if (argument.second->nodeType() == NodeType::Accessor) {
            auto acc = std::dynamic_pointer_cast<AST::Accessor>(argument.second);
            acc->narrowTo(type);
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
              if (argument.second->nodeType() == NodeType::Fetch) {
                auto fetch = std::dynamic_pointer_cast<AST::Fetch>(argument.second);
                fetch->narrowTo(type);
              } else if (argument.second->nodeType() == NodeType::Accessor) {
                auto acc = std::dynamic_pointer_cast<AST::Accessor>(argument.second);
                acc->narrowTo(type);
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
          argumentsInOrder[funcArgIndex] = { argument.first, std::vector<std::shared_ptr<ExpressionNode>>() };
        }
        ALTACORE_VARIANT_GET<std::vector<std::shared_ptr<ExpressionNode>>>(argumentsInOrder[funcArgIndex].second).push_back(argument.second);
      } else {
        argumentsInOrder[funcArgIndex] = argument;
      }
      argMap[i] = funcArgIndex;
      if (!std::get<2>(targetType->parameters[funcArgIndex])) {
        funcArgIndex++;
      }
    }
    if (!ok) continue;
    found = true;
    std::vector<ALTACORE_VARIANT<std::shared_ptr<ExpressionNode>, std::vector<std::shared_ptr<ExpressionNode>>>> args(argumentsInOrder.size(), nullptr);
    for (auto& [i, arg]: argumentsInOrder) {
      args[i] = arg.second;
    }
    compatibles.push_back({ compatiblities, targetType, args, argMap });
  }

  std::vector<size_t> mostCompatibleCompatiblities;
  std::shared_ptr<DET::Type> mostCompatibleType = nullptr;
  std::vector<ALTACORE_VARIANT<std::shared_ptr<ExpressionNode>, std::vector<std::shared_ptr<ExpressionNode>>>> mostCompatibleArguments;
  std::unordered_map<size_t, size_t> mostCompatibleArgMap;
  for (auto& [compatabilities, type, args, argMap]: compatibles) {
    if (mostCompatibleCompatiblities.size() > 0) {
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
        mostCompatibleType = type;
        mostCompatibleCompatiblities = compatabilities;
        mostCompatibleArguments = args;
        mostCompatibleArgMap = argMap;
      }
    } else {
      mostCompatibleType = type;
      mostCompatibleCompatiblities = compatabilities;
      mostCompatibleArguments = args;
      mostCompatibleArgMap = argMap;
    }
  }

  if (mostCompatibleType != nullptr) {
    $targetType = mostCompatibleType;
    if (target->nodeType() == NodeType::Fetch) {
      auto fetch = std::dynamic_pointer_cast<AST::Fetch>(target);
      fetch->narrowTo(mostCompatibleType);
    } else if (target->nodeType() == NodeType::Accessor) {
      auto acc = std::dynamic_pointer_cast<AST::Accessor>(target);
      acc->narrowTo(mostCompatibleType);
    }
  }

  if (mostCompatibleType == nullptr || !found) {
    if (possible) {
      throw std::runtime_error("No functions matching the given arguments found in the given expression");
    } else {
      throw std::runtime_error("No functions found in the given expression");
    }
  }

  $adjustedArguments = mostCompatibleArguments;

  $argumentMap = mostCompatibleArgMap;
};
