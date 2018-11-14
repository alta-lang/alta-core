#include "../include/altacore/util.hpp"

bool AltaCore::Util::isInFunction(AltaCore::DET::ScopeItem* item) {
  auto maybeScope = item->parentScope;
  
  while (!maybeScope.expired()) {
    auto scope = maybeScope.lock();
    if (!scope->parentFunction.expired()) {
      return true;
    } else if (!scope->parent.expired()) {
      maybeScope = scope->parent;
    } else {
      return false;
    }
  }

  // just in case
  return false;
};

std::weak_ptr<AltaCore::DET::Module> AltaCore::Util::getModule(AltaCore::DET::Scope* scope) {
  if (scope == nullptr) {
    return std::weak_ptr<AltaCore::DET::Module>();
  }
  if (!scope->parentModule.expired()) {
    return scope->parentModule;
  }
  if (!scope->parentFunction.expired()) {
    auto func = scope->parentFunction.lock();
    if (!func->parentScope.expired()) {
      return getModule(func->parentScope.lock().get());
    }
  }
  if (!scope->parent.expired()) {
    return getModule(scope->parent.lock().get());
  }
  return std::weak_ptr<AltaCore::DET::Module>();
};
