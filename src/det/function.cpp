#include "../../include/altacore/det/function.hpp"

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
    auto& [name, type] = param;
    self->parameters.push_back(std::make_tuple(name, std::dynamic_pointer_cast<Type>(type->deepClone())));
  }
  self->parameterVariables.clear();
  for (auto& paramVar: parameterVariables) {
    self->parameterVariables.push_back(std::dynamic_pointer_cast<Variable>(paramVar->deepClone()));
  }
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());
  self->scope->parentFunction = self;
  return self;
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Function::create(std::shared_ptr<AltaCore::DET::Scope> parentScope, std::string name, std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>>> parameters, std::shared_ptr<AltaCore::DET::Type> returnType) {
  auto func = std::make_shared<Function>(parentScope, name);
  func->parameters = parameters;
  func->returnType = returnType;
  func->scope = std::make_shared<Scope>(func);

  for (auto& [name, type]: parameters) {
    auto var = std::make_shared<Variable>(name, type);
    var->parentScope = func->scope;
    func->parameterVariables.push_back(var);
    func->scope->items.push_back(var);
  }

  return func;
};

AltaCore::DET::Function::Function(std::shared_ptr<AltaCore::DET::Scope> _parentScope, std::string _name):
  ScopeItem(_name, _parentScope)
  {};
