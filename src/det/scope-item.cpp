#include "../../include/altacore/det/scope-item.hpp"
#include "../../include/altacore/ast.hpp"

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
{
  if (_parentScope) {
    itemID = _parentScope->nextItemID++;
  }
  if (auto scope = parentScope.lock()) {
    if (auto mod = scope->parentModule.lock()) {
      moduleIndex = mod->rootItemCount++;
    }
  }
};

std::vector<std::shared_ptr<AltaCore::DET::ScopeItem>> AltaCore::DET::ScopeItem::getUnderlyingItems(std::shared_ptr<AltaCore::DH::Node> node) {
  if (auto fetch = std::dynamic_pointer_cast<DH::Fetch>(node)) {
    return fetch->items;
  } else if (auto acc = std::dynamic_pointer_cast<DH::Accessor>(node)) {
    return acc->items;
  }

  return {};
};
