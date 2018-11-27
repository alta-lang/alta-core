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
