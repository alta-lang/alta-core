#include "../../include/altacore/det/function.hpp"
#include "../../include/altacore/det/class.hpp"
#include "../../include/altacore/ast/function-definition-node.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Function::nodeType() {
  return NodeType::Function;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Function::clone() {
  return std::make_shared<Function>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Function::deepClone() {
  auto self = std::dynamic_pointer_cast<Function>(clone());
  self->parameters.clear();
  for (auto& param: parameters) {
    auto& [name, type, isVariable, id] = param;
    self->parameters.push_back(std::make_tuple(name, std::dynamic_pointer_cast<Type>(type->deepClone()), isVariable, id));
  }
  self->parameterVariables.clear();
  for (auto& paramVar: parameterVariables) {
    self->parameterVariables.push_back(std::dynamic_pointer_cast<Variable>(paramVar->deepClone()));
  }
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());
  self->scope->parentFunction = self;
  return self;
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Function::create(std::shared_ptr<AltaCore::DET::Scope> parentScope, std::string name, std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> parameters, std::shared_ptr<AltaCore::DET::Type> returnType) {
  auto func = std::make_shared<Function>(parentScope, name);
  func->parameters = parameters;
  func->returnType = returnType;
  func->scope = std::make_shared<Scope>(func);

  for (auto& [name, type, isVariable, id]: parameters) {
    auto var = std::make_shared<Variable>(name, isVariable ? type->point() : type);
    var->parentScope = func->scope;
    var->isVariable = isVariable;
    func->parameterVariables.push_back(var);
    func->scope->items.push_back(var);

    func->publicHoistedItems.push_back(type);
  }

  if (returnType) {
    func->publicHoistedItems.push_back(returnType);
  }

  return func;
};

AltaCore::DET::Function::Function(std::shared_ptr<AltaCore::DET::Scope> _parentScope, std::string _name):
  ScopeItem(_name, _parentScope)
  {};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Function::instantiateGeneric(std::vector<std::shared_ptr<Type>> genericArguments) {
  if (auto func = ast.lock()) {
    auto inf = info.lock();
    if (!inf) {
      return nullptr;
    }
    return func->instantiateGeneric(inf, genericArguments);
  } else {
    return nullptr;
  }
};

void AltaCore::DET::Function::recreate(std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> _parameters, std::shared_ptr<Type> _returnType) {
  parameters = _parameters;
  returnType = _returnType;

  for (auto& var: parameterVariables) {
    for (size_t i = 0; i < scope->items.size(); ++i) {
      if (scope->items[i]->id == var->id) {
        scope->items.erase(scope->items.begin() + 1);
        --i;
      }
    }
  }

  parameterVariables.clear();

  for (auto& [name, type, isVariable, id]: parameters) {
    auto var = std::make_shared<Variable>(name, isVariable ? type->point() : type);
    var->parentScope = scope;
    var->isVariable = isVariable;
    parameterVariables.push_back(var);
    scope->items.push_back(var);

    publicHoistedItems.push_back(type);
  }

  if (returnType) {
    publicHoistedItems.push_back(returnType);
  }
};

bool AltaCore::DET::Function::isVirtual()  {
  auto pScope = parentScope.lock();
  if (!pScope || (pScope && !pScope->parentClass.lock())) {
    return false;
  }
  if (_virtual) {
    return true;
  }
  auto pClass = pScope->parentClass.lock();
  auto thisType = Type::getUnderlyingType(shared_from_this());

  std::function<bool(std::shared_ptr<Class>)> loop = [&](std::shared_ptr<Class> klass) {
    for (auto& item: klass->scope->items) {
      if (item->nodeType() != NodeType::Function) continue;
      
      auto func = std::dynamic_pointer_cast<DET::Function>(item);

      if (!func) continue;

      if (func->name != name) continue;

      if (!func->isVirtual()) continue;

      if (func->isAccessor != isAccessor) continue;
      if (func->isOperator != isOperator) continue;

      /**
       * TODO: allow C++-style covariant overrides, e.g:
       * class Parent {
       *   public @virtual function hello(): ref Parent {
       *     return this
       *   }
       * }
       * 
       * class Child {
       *   public @virtual function hello(): ref Child {
       *     return this
       *   }
       * }
       */
      if (isAccessor) {
        if (*func->returnType == *thisType->returnType) return true;
      } else {
        if (*Type::getUnderlyingType(func) == *thisType) return true;
      }
    }

    for (auto& parent: klass->parents) {
      if (loop(parent)) return true;
    }

    return false;
  };

  for (auto& parent: pClass->parents) {
    if (loop(parent)) return true;
  }

  return false;
};

std::string AltaCore::DET::Function::toString() const {
  std::string result = name;
  if (genericArguments.size() == genericParameterCount) {
    if (genericArguments.size() > 0) {
      result += '<';
      bool isFirst = true;
      for (auto& genArg: genericArguments) {
        if (isFirst) {
          isFirst = false;
        } else {
          result += ", ";
        }
        result += genArg->toString();
      }
      result += '>';
    }
    result += '(';
    bool isFirst = true;
    for (auto& [name, type, isVariable, id]: parameters) {
      if (isFirst) {
        isFirst = false;
      } else {
        result += ", ";
      }
      result += name + ": " + type->toString();
      if (isVariable) {
        result += "...";
      }
    }
    result += "): ";
    bool doIt = true;
    if (returnType->klass) {
      if (auto classParent = returnType->klass->parentScope.lock()) {
        if (classParent->id == scope->id) {
          doIt = false;
          if (isAsync) {
            result += "@Coroutine@";
          } else if (isGenerator) {
            result += "@Generator@";
          } else {
            result += "@CaptureClass@";
          }
        }
      }
    }
    if (doIt) {
      result += returnType->toString();
    }
  }

  result = (parentScope.lock() ? parentScope.lock()->toString() : "") + '.' + result;

  return result;
};

auto AltaCore::DET::Function::fullPrivateHoistedItems() const -> std::vector<std::shared_ptr<ScopeItem>> {
  auto result = privateHoistedItems;

  for (auto& item: scope->items) {
    auto priv = item->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  return result;
};
