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
  std::vector<std::tuple<std::vector<size_t>, std::shared_ptr<DET::Type>, std::vector<std::shared_ptr<ExpressionNode>>>> compatibles;
  for (auto& targetType: targetTypes) {
    if (!targetType->isFunction) continue;
    possible = true;
    if (targetType->parameters.size() != arguments.size()) continue;
    std::unordered_map<size_t, std::pair<std::string, std::shared_ptr<ExpressionNode>>> argumentsInOrder;
    int funcArgIndex = 0;
    for (size_t i = 0; i < arguments.size(); i++) {
      if (arguments[i].first.empty()) {
        if (funcArgIndex >= targetType->parameters.size()) {
          throw std::runtime_error("welp, the function doesn't have any arguments after the previous argument");
        }
      } else {
        bool found = false;
        for (int j = 0; j < targetType->parameters.size(); j++) {
          if (targetType->parameters[j].first == arguments[i].first) {
            funcArgIndex = j;
            found = true;
            break;
          }
        }
        if (!found) {
          throw std::runtime_error("named argument not found");
        }
      }
      argumentsInOrder[funcArgIndex] = arguments[i];
      funcArgIndex++;
    }
    bool ok = true;
    std::vector<size_t> compatiblities(argumentsInOrder.size(), 0);
    for (auto& [i, argument]: argumentsInOrder) {
      auto types = DET::Type::getUnderlyingTypes(argument.second.get());
      size_t compatiblity = 0;
      for (auto& type: types) {
        auto currentCompat = targetType->parameters[i].second->compatiblity(*type);
        if (currentCompat > compatiblity) {
          compatiblity = currentCompat;
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
      compatiblities[i] = compatiblity;
    }
    if (!ok) continue;
    found = true;
    std::vector<std::shared_ptr<ExpressionNode>> args(argumentsInOrder.size(), nullptr);
    for (auto&[i, arg] : argumentsInOrder) {
      args[i] = arg.second;
    }
    compatibles.push_back({ compatiblities, targetType, args });
  }

  std::vector<size_t> mostCompatibleCompatiblities;
  std::shared_ptr<DET::Type> mostCompatibleType = nullptr;
  std::vector<std::shared_ptr<ExpressionNode>> mostCompatibleArguments;
  for (auto& [compatabilities, type, args]: compatibles) {
    if (mostCompatibleCompatiblities.size() > 0) {
      size_t numberGreater = 0;
      for (size_t i = 0; i < compatabilities.size(); i++) {
        if (compatabilities[i] > mostCompatibleCompatiblities[i]) {
          numberGreater++;
        }
      }
      if (numberGreater > 0) {
        mostCompatibleType = type;
        mostCompatibleCompatiblities = compatabilities;
        mostCompatibleArguments = args;
      }
    } else {
      mostCompatibleType = type;
      mostCompatibleCompatiblities = compatabilities;
      mostCompatibleArguments = args;
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
};
