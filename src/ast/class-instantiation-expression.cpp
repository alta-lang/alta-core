#include "../../include/altacore/ast/class-instantiation-expression.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/det/function.hpp"
#include "../../include/altacore/ast/function-call-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassInstantiationExpression::nodeType() {
  return NodeType::ClassInstantiationExpression;
};

void AltaCore::AST::ClassInstantiationExpression::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  target->detail(scope);

  if (target->nodeType() == NodeType::Fetch) {
    auto fetch = std::dynamic_pointer_cast<Fetch>(target);
    if (!fetch->$narrowedTo) {
      throw std::runtime_error("the target must be narrowed before it can be instantiated");
    }
    $klass = std::dynamic_pointer_cast<DET::Class>(fetch->$narrowedTo);
  } else if (target->nodeType() == NodeType::Accessor) {
    auto acc = std::dynamic_pointer_cast<Accessor>(target);
    if (!acc->$narrowedTo) {
      throw std::runtime_error("the target must be narrowed before it can be instantiated");
    }
    $klass = std::dynamic_pointer_cast<DET::Class>(acc->$narrowedTo);
  } else {
    throw std::runtime_error("invalid target retrieval node for class instantiation");
  }

  if (!$klass) {
    throw std::runtime_error("invalid target for class instantiation. most likely, the target was not a class");
  }

  std::vector<std::shared_ptr<DET::Type>> targetTypes;
  for (auto& constr: $klass->constructors) {
    targetTypes.push_back(std::make_shared<DET::Type>(constr->returnType, constr->parameters));
  }

  auto [index, argMap, adjArgs] = FunctionCallExpression::findCompatibleCall(arguments, targetTypes);

  if (index != SIZE_MAX) {
    $constructor = $klass->constructors[index];
    $adjustedArguments = adjArgs;
    $argumentMap = argMap;
  } else {
    throw std::runtime_error("unable to find suitable constructor");
  }
};

ALTACORE_AST_VALIDATE_D(ClassInstantiationExpression) {
  
};