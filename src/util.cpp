#include "../include/altacore/util.hpp"
#include <stdexcept>
#include <cctype>

bool AltaCore::Util::isInFunction(const AltaCore::DET::ScopeItem* item) {
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
  if (auto klass = scope->parentClass.lock()) {
    if (auto parent = klass->parentScope.lock()) {
      return getModule(parent.get());
    }
  }
  return std::weak_ptr<AltaCore::DET::Module>();
};

std::weak_ptr<AltaCore::DET::Function> AltaCore::Util::getFunction(std::shared_ptr<AltaCore::DET::Scope> scope) {
  if (scope == nullptr) {
    return std::weak_ptr<AltaCore::DET::Function>();
  }
  if (!scope->parentFunction.expired()) {
    return scope->parentFunction;
  }
  if (!scope->parentModule.expired()) {
    return std::weak_ptr<AltaCore::DET::Function>();
  }
  if (!scope->parent.expired()) {
    return getFunction(scope->parent.lock());
  }
  if (auto ns = scope->parentNamespace.lock()) {
    if (auto parent = ns->parentScope.lock()) {
      return getFunction(parent);
    }
  }
  if (auto klass = scope->parentClass.lock()) {
    if (auto parent = klass->parentScope.lock()) {
      return getFunction(parent);
    }
  }
  return std::weak_ptr<AltaCore::DET::Function>();
};

std::weak_ptr<AltaCore::DET::Class> AltaCore::Util::getClass(std::shared_ptr<const AltaCore::DET::Scope> scope) {
  if (scope == nullptr) {
    return std::weak_ptr<AltaCore::DET::Class>();
  }
  if (!scope->parentClass.expired()) {
    return scope->parentClass;
  }
  if (!scope->parentModule.expired()) {
    return std::weak_ptr<AltaCore::DET::Class>();
  }
  if (!scope->parent.expired()) {
    return getClass(scope->parent.lock());
  }
  if (auto ns = scope->parentNamespace.lock()) {
    if (auto parent = ns->parentScope.lock()) {
      return getClass(parent);
    }
  }
  if (auto func = scope->parentFunction.lock()) {
    if (auto parent = func->parentScope.lock()) {
      return getClass(parent);
    }
  }
  return std::weak_ptr<AltaCore::DET::Class>();
};

std::string AltaCore::Util::unescape(const std::string& data) {
  std::string result;

  for (size_t i = 0; i < data.length(); i++) {
    auto& character = data[i];
    if (character == '\\' && i + 1 < data.length()) {
      const char nextChar = data[i + 1];
      if (nextChar == 't') {
        result += '\t';
      } else if (nextChar == 'r') {
        result += '\r';
      } else if (nextChar == 'n') {
        result += '\n';
      } else if (nextChar == 'x' && i + 3 < data.length()) {
        const uint8_t upperHalf = hexDigitToDecimal(data[i + 2]);
        const uint8_t lowerHalf = hexDigitToDecimal(data[i + 3]);
        result += (char)((upperHalf << 4) | lowerHalf);

        // skip the 2 next characters
        i++;
        i++;
      } else {
        result += nextChar;
      }
      i++; // skip the next character
    } else {
      result += character;
    }
  }

  return result;
};

std::string AltaCore::Util::escape(const std::string& data) {
  std::string result;

  for (auto& ch: data) {
    // all printable characters except \ and "
    if (
      ch >= ' ' &&
      ch <= '~' &&
      ch != '\\' &&
      ch != '"'
    ) {
      result += ch;
    } else {
      result += '\\';
      if (ch == '\\' || ch == '"') {
        result += ch;
      } else if (ch == '\t') {
        result += 't';
      } else if (ch == '\r') {
        result += 'r';
      } else if (ch == '\n') {
        result += 'n';
      } else {
        result += 'x';
        result += hexDigits[ch >> 4];  // upper half
        result += hexDigits[ch & 0xf]; // lower half
      }
    }
  }

  return result;
};

uint8_t AltaCore::Util::hexDigitToDecimal(const char singleDigit) {
  if (singleDigit >= '0' && singleDigit <= '9') {
    return singleDigit - 0x30; // according to asciitable.com, '0' starts at 0x30
  } else if (singleDigit >= 'A' && singleDigit <= 'F') {
    return singleDigit - 0x41;
  } else if (singleDigit >= 'a' && singleDigit <= 'f') {
    return singleDigit - 0x61;
  } else {
    throw std::runtime_error(std::string("invalid hex digit: ") + singleDigit);
  }
};

bool AltaCore::Util::stringsAreEqualCaseInsensitive(const std::string& lhs, const std::string& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (size_t i = 0; i < lhs.size(); i++) {
    if (std::tolower(lhs[i]) != std::tolower(rhs[i])) {
      return false;
    }
  }
  return true;
};

void AltaCore::Util::exportClassIfNecessary(std::shared_ptr<AltaCore::DET::Scope> scope, std::shared_ptr<AltaCore::DET::Type> type, bool force) {
  auto parentFunc = getFunction(scope).lock();
  auto parentClass = getClass(scope).lock();
  bool found = false;
  if (force) found = true;
  if (parentFunc && parentFunc->isExport) found = true;
  if (parentFunc && parentFunc->isMethod && getClass(parentFunc->parentScope.lock()).lock()->isExport) found = true;
  if (parentClass && parentClass->isExport) found = true;
  if (!found) return;
  if (type->klass) {
    auto thisMod = getModule(scope.get()).lock();
    auto thatMod = getModule(type->klass->parentScope.lock().get()).lock();
    if (thisMod->id == thatMod->id) {
      // same module
      if (!type->klass->isExport) {
        type->klass->isExport = true;
      }
    }
  }
};

std::shared_ptr<AltaCore::DET::Scope> AltaCore::Util::findLoopScope(std::shared_ptr<AltaCore::DET::Scope> scope) {
  if (scope->isLoopScope) return scope;
  auto tmp = scope;
  while (auto parentScope = tmp->parent.lock()) {
    if (parentScope->isLoopScope) return parentScope;
    tmp = parentScope;
  }
  return nullptr;
};
