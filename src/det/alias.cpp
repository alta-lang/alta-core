#include "../../include/altacore/det/alias.hpp"

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
  std::shared_ptr<AltaCore::DET::Scope> _parentScope
):
  ScopeItem(_name, _parentScope),
  target(_target)
  {};