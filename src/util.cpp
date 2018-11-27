#include "../include/altacore/util.hpp"

bool AltaCore::Util::isInFunction(AltaCore::DET::ScopeItem* item) {
  auto maybeScope = item->parentScope;
  
  while (!maybeScope.expired()) {
    auto scope = maybeScope.lock();
    if (!scope->parentFunction.expired()) {
      return true;
    } else if (!scope->parent.expired()) {
      maybeScope = scope->parent;
    } else if (!scope->parentNamespace.expired()) {
      maybeScope = scope->parentNamespace.lock()->parentScope;
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
  if (auto ns = scope->parentNamespace.lock()) {
    if (auto parent = ns->parentScope.lock()) {
      return getModule(parent.get());
    }
  }
  return std::weak_ptr<AltaCore::DET::Module>();
};

std::string AltaCore::Util::unescape(std::string data) {
  std::string result;

  for (size_t i = 0; i < data.length(); i++) {
    auto& character = data[i];
    if (character == '\\' && i + 1 < data.length()) {
      result += data[i + 1];
      i++; // skip the next character
    } else {
      result += character;
    }
  }

  return result;
};
