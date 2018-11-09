#include "../include/altacore/det/function.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Function::nodeType() {
  return NodeType::Function;
};

AltaCore::DET::Function* AltaCore::DET::Function::clone() {
  return new Function(*this);
};

AltaCore::DET::Function* AltaCore::DET::Function::deepClone() {
  Function* self = clone();
  self->parameters.clear();
  for (auto& param: parameters) {
    auto& [name, type] = param;
    self->parameters.push_back(std::make_tuple(name, type->deepClone()));
  }
  self->parameterVariables.clear();
  for (auto& paramVar: parameterVariables) {
    self->parameterVariables.push_back(paramVar->deepClone());
  }
  self->scope = scope->deepClone();
  return self;
};

AltaCore::DET::Function::Function(AltaCore::DET::Scope* _parentScope, std::string _name, std::vector<std::tuple<std::string, AltaCore::DET::Type*>> _parameters, AltaCore::DET::Type* _returnType):
  ScopeItem(_name, _parentScope),
  parameters(_parameters),
  returnType(_returnType),
  scope(new Scope(this))
{
  for (auto& param: parameters) {
    parameterVariables.push_back(new Variable(std::get<0>(param), std::get<1>(param)->deepClone()));
  }
};