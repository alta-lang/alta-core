#include "../../include/altacore/det/scope-item.hpp"

const AltaCore::DET::NodeType AltaCore::DET::ScopeItem::nodeType() {
  return NodeType::ScopeItem;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::ScopeItem::clone() {
  return std::make_shared<ScopeItem>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::ScopeItem::deepClone() {
  // we contain a reference to our parent scope, but we don't clone it on `deepClone`,
  // that's the parent scope's job (or if the clone was initiated directly on the scope item,
  // then it's the initiator's job)
  return clone();
};

AltaCore::DET::ScopeItem::ScopeItem(std::string _name, std::shared_ptr<AltaCore::DET::Scope> _parentScope):
  name(_name),
  parentScope(_parentScope)
  {};
