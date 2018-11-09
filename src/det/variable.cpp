#include "../include/altacore/det/variable.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Variable::nodeType() {
  return NodeType::Variable;
};

AltaCore::DET::Variable* AltaCore::DET::Variable::clone() {
  return new Variable(*this);
};

AltaCore::DET::Variable* AltaCore::DET::Variable::deepClone() {
  Variable* self = clone();
  self->type = type->deepClone();
  return self;
};

AltaCore::DET::Variable::Variable(std::string _name, AltaCore::DET::Type* _type, AltaCore::DET::Scope* _parentScope):
  ScopeItem(_name, _parentScope),
  type(_type)
  {};
