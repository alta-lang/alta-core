#include "../include/altacore/det/scope.hpp"
#include "../include/altacore/det/function.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Scope::nodeType() {
  return NodeType::Scope;
};

AltaCore::DET::Scope* AltaCore::DET::Scope::clone() {
  return new Scope(*this);
};

AltaCore::DET::Scope* AltaCore::DET::Scope::deepClone() {
  Scope* self = clone();
  self->items.clear();
  for (auto& item: items) {
    auto newItem = item->deepClone();
    newItem->parentScope = self;
    self->items.push_back(newItem);
  }
  return self;
};

AltaCore::DET::Scope::Scope() {};
AltaCore::DET::Scope::Scope(AltaCore::DET::Scope* _parent):
  parent(_parent),
  relativeID(parent->nextChildID)
{
  parent->nextChildID++;
};
AltaCore::DET::Scope::Scope(AltaCore::DET::Module* _parentModule):
  parentModule(_parentModule)
  {};
AltaCore::DET::Scope::Scope(AltaCore::DET::Function* _parentFunction):
  parentFunction(_parentFunction)
  {};

std::vector<AltaCore::DET::ScopeItem*> AltaCore::DET::Scope::findAll(std::string name) {
  std::vector<AltaCore::DET::ScopeItem*> result;

  for (auto& item: items) {
    if (item->name == name) {
      result.push_back(item);
    }
  }

  Scope* parentScope = nullptr;
  if (parent != nullptr) {
    parentScope = parent;
  } else if (parentFunction != nullptr && parentFunction->parentScope != nullptr) {
    parentScope = parentFunction->parentScope;
  }

  if (parentScope != nullptr) {
    auto other = parentScope->findAll(name);
    result.insert(result.end(), other.begin(), other.end());
  }

  return result;
};