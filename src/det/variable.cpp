#include "../../include/altacore/det/variable.hpp"
#include "../../include/altacore/det/scope.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Variable::nodeType() {
  return NodeType::Variable;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Variable::clone() {
  return std::make_shared<Variable>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Variable::deepClone() {
  auto self = std::dynamic_pointer_cast<Variable>(clone());
  self->type = std::dynamic_pointer_cast<Type>(type->deepClone());
  return self;
};

AltaCore::DET::Variable::Variable(
  std::string _name,
  std::shared_ptr<AltaCore::DET::Type> _type,
  AltaCore::Errors::Position position,
  std::shared_ptr<AltaCore::DET::Scope> _parentScope
):
  ScopeItem(_name, position, _parentScope),
  type(_type)
  {};

std::string AltaCore::DET::Variable::toString() const {
  return (parentScope.lock() ? parentScope.lock()->toString() : "") + '.' + name + ": " + type->toString();
};
