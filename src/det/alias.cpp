#include "../../include/altacore/det/alias.hpp"
#include "../../include/altacore/det/scope.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Alias::nodeType() {
  return NodeType::Alias;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Alias::clone() {
  return std::make_shared<Alias>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Alias::deepClone() {
  auto self = std::dynamic_pointer_cast<Alias>(clone());
  self->target = std::dynamic_pointer_cast<ScopeItem>(target->deepClone());
  return self;
};

AltaCore::DET::Alias::Alias(
  std::string _name,
  std::shared_ptr<AltaCore::DET::ScopeItem> _target,
  AltaCore::Errors::Position position,
  std::shared_ptr<AltaCore::DET::Scope> _parentScope
):
  ScopeItem(_name, position, _parentScope),
  target(_target)
  {};

std::string AltaCore::DET::Alias::toString() const {
  std::string result = '[' + name + " (alias for { " + target->toString() + " })]";

  result = Util::joinDETPaths({ (parentScope.lock() ? parentScope.lock()->toString() : ""), result });

  return result;
};
