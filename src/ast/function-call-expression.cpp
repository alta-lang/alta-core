#include "../../include/altacore/ast/function-call-expression.hpp"
#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"

const AltaCore::AST::NodeType AltaCore::AST::FunctionCallExpression::nodeType() {
  return NodeType::FunctionCallExpression;
};

AltaCore::AST::FunctionCallExpression::FunctionCallExpression(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::vector<std::shared_ptr<AltaCore::AST::ExpressionNode>> _arguments):
  target(_target),
  arguments(_arguments)
  {};

void AltaCore::AST::FunctionCallExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);

  for (auto& arg: arguments) {
    arg->detail(scope);
  }

  auto targetTypes = DET::Type::getUnderlyingTypes(target.get());
  if (targetTypes.size() < 1) {
    throw std::runtime_error("Can't call an unknown expression like a function");
  }

  bool found = false; // whether we found the right function
  bool possible = false; // whether we found any function at all
  std::vector<std::pair<std::vector<size_t>, std::shared_ptr<DET::Type>>> compatibles;
  for (auto& targetType: targetTypes) {
    if (!targetType->isFunction) continue;
    possible = true;
    if (targetType->parameters.size() != arguments.size()) continue;
    bool ok = true;
    std::vector<size_t> compatiblities;
    for (size_t i = 0; i < arguments.size(); i++) {
      auto types = DET::Type::getUnderlyingTypes(arguments[i].get());
      size_t compatiblity = 0;
      for (auto& type: types) {
        auto currentCompat = targetType->parameters[i]->compatiblity(*type);
        if (currentCompat > compatiblity) {
          compatiblity = currentCompat;
          if (arguments[i]->nodeType() == NodeType::Fetch) {
            auto fetch = std::dynamic_pointer_cast<AST::Fetch>(arguments[i]);
            fetch->narrowTo(type);
          } else if (arguments[i]->nodeType() == NodeType::Accessor) {
            auto acc = std::dynamic_pointer_cast<AST::Accessor>(arguments[i]);
            acc->narrowTo(type);
          }
        }
      }
      if (compatiblity == 0) {
        ok = false;
        break;
      }
      compatiblities.push_back(compatiblity);
    }
    if (!ok) continue;
    found = true;
    compatibles.push_back({ compatiblities, targetType });
  }

  std::vector<size_t> mostCompatibleCompatiblities;
  std::shared_ptr<DET::Type> mostCompatibleType = nullptr;
  for (auto& [compatabilities, type]: compatibles) {
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
      }
    } else {
      mostCompatibleType = type;
      mostCompatibleCompatiblities = compatabilities;
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
};
