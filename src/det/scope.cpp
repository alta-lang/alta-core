#include "../../include/altacore/det/scope.hpp"
#include "../../include/altacore/det/function.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Scope::nodeType() {
  return NodeType::Scope;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Scope::clone() {
  return std::make_shared<Scope>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Scope::deepClone() {
  auto self = std::dynamic_pointer_cast<Scope>(clone());
  self->items.clear();
  for (auto& item: items) {
    auto newItem = std::dynamic_pointer_cast<ScopeItem>(item->deepClone());
    newItem->parentScope = self;
    self->items.push_back(newItem);
  }
  return self;
};

AltaCore::DET::Scope::Scope() {};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Scope> _parent):
  parent(_parent),
  relativeID(parent.lock()->nextChildID)
{
  parent.lock()->nextChildID++;
};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Module> _parentModule):
  parentModule(_parentModule)
  {};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Function> _parentFunction):
  parentFunction(_parentFunction)
  {};

std::vector<std::shared_ptr<AltaCore::DET::ScopeItem>> AltaCore::DET::Scope::findAll(std::string name) {
  std::vector<std::shared_ptr<AltaCore::DET::ScopeItem>> result;

  for (auto& item: items) {
    if (item->name == name) {
      result.push_back(item);
    }
  }

  std::shared_ptr<Scope> parentScope = nullptr;
  if (!parent.expired()) {
    parentScope = parent.lock();
  } else if (!parentFunction.expired() && !parentFunction.lock()->parentScope.expired()) {
    parentScope = parentFunction.lock()->parentScope.lock();
  }

  if (parentScope != nullptr) {
    auto other = parentScope->findAll(name);
    result.insert(result.end(), other.begin(), other.end());
  }

  return result;
};
