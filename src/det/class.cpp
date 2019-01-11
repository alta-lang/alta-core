#include "../../include/altacore/det/class.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Class::nodeType() {
  return NodeType::Class;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Class::clone() {
  return std::make_shared<Class>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Class::deepClone() {
  auto self = std::dynamic_pointer_cast<Class>(clone());
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());

  return self;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::DET::Class::create(std::string name, std::shared_ptr<AltaCore::DET::Scope> parentScope) {
  auto klass = std::make_shared<Class>(name, parentScope);
  klass->scope = std::make_shared<Scope>(klass);

  return klass;
};

AltaCore::DET::Class::Class(std::string _name, std::shared_ptr<AltaCore::DET::Scope> _parentScope):
  ScopeItem(_name, _parentScope)
  {};
