#include "../../include/altacore/ast/class-instantiation-expression.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/det/function.hpp"
#include "../../include/altacore/ast/function-call-expression.hpp"
#include "../../include/altacore/simple-map.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassInstantiationExpression::nodeType() {
  return NodeType::ClassInstantiationExpression;
};

ALTACORE_AST_DETAIL_D(ClassInstantiationExpression) {
  ALTACORE_MAKE_DH(ClassInstantiationExpression);
  info->target = target->fullDetail(scope);

  info->persistent = persistent;

  if (auto fetch = std::dynamic_pointer_cast<DH::Fetch>(info->target)) {
    if (!fetch->narrowedTo) {
      ALTACORE_DETAILING_ERROR("the target must be narrowed before it can be instantiated");
    }
    info->klass = std::dynamic_pointer_cast<DET::Class>(fetch->narrowedTo);
  } else if (auto acc = std::dynamic_pointer_cast<DH::Accessor>(info->target)) {
    if (!acc->narrowedTo) {
      ALTACORE_DETAILING_ERROR("the target must be narrowed before it can be instantiated");
    }
    info->klass = std::dynamic_pointer_cast<DET::Class>(acc->narrowedTo);
  } else if (auto sup = std::dynamic_pointer_cast<DH::SuperClassFetch>(info->target)) {
    info->klass = sup->superclass;
    info->superclass = true;
  } else if (auto special = std::dynamic_pointer_cast<DH::SpecialFetchExpression>(info->target)) {
    if (special->items.size() != 1) {
      ALTACORE_DETAILING_ERROR("the target must be narrowed before it can be instantiated");
    }
    auto trueItem = special->items[0];
    while (trueItem->nodeType() == DET::NodeType::Alias) {
      trueItem = std::dynamic_pointer_cast<DET::Alias>(trueItem)->target;
    }
    info->klass = std::dynamic_pointer_cast<DET::Class>(trueItem);
  } else {
    ALTACORE_DETAILING_ERROR("invalid target retrieval node for class instantiation");
  }

  if (!info->klass) {
    ALTACORE_DETAILING_ERROR("invalid target for class instantiation. most likely, the target was not a class");
  }

  std::vector<std::shared_ptr<DET::Type>> targetTypes;
  ALTACORE_MAP<size_t, size_t> indexMap;
  for (size_t i = 0; i < info->klass->constructors.size(); i++) {
    auto& constr = info->klass->constructors[i];
    if (!scope->canSee(constr)) {
      continue;
    }
    indexMap[targetTypes.size()] = i;
    targetTypes.push_back(std::make_shared<DET::Type>(constr->returnType, constr->parameters));
  }

  std::vector<std::tuple<std::string, std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>> argsWithDet;

  for (auto& [name, arg]: arguments) {
    auto det = arg->fullDetail(scope);
    argsWithDet.emplace_back(name, arg, det);
    info->arguments.push_back(det);
  }

  auto [index, argMap, adjArgs] = FunctionCallExpression::findCompatibleCall(argsWithDet, targetTypes);

  if (index != SIZE_MAX) {
    info->constructor = info->klass->constructors[indexMap[index]];
    info->adjustedArguments = adjArgs;
    info->argumentMap = argMap;
    info->inputScope->hoist(info->constructor);
  } else {
    std::string message = "Unable to find suitable constructor\n";
    message += "Argument types are: (";
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
    message += "Available constructor signatures:\n";
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
  return info;
};

ALTACORE_AST_VALIDATE_D(ClassInstantiationExpression) {
  ALTACORE_VS_S(ClassInstantiationExpression);
  if (!target) ALTACORE_VALIDATION_ERROR("empty target for class instantiation");
  target->validate(stack, info->target);
  for (size_t i = 0; i < arguments.size(); i++) {
    auto& [name, arg] = arguments[i];
    auto& argDet = info->arguments[i];
    if (!arg) ALTACORE_VALIDATION_ERROR("empty argument for class instantiation");
    arg->validate(stack, argDet);
  }
  if (!info->klass) ALTACORE_VALIDATION_ERROR("failed to find desired class for class instantiation");
  if (!info->constructor) ALTACORE_VALIDATION_ERROR("failed to find proper constructor during detailing of class instantiation");
  // TODO: validate adjusted arguments
  ALTACORE_VS_E;
};
