#ifndef ALTACORE_UTIL_HPP
#define ALTACORE_UTIL_HPP

#include "ast.hpp"
#include "det.hpp"

namespace AltaCore {
  namespace Util {
    static const char* const hexDigits = "0123456789abcdef";
    
    bool isInFunction(AltaCore::DET::ScopeItem* item);
    std::weak_ptr<DET::Module> getModule(AltaCore::DET::Scope* scope);
    std::string unescape(std::string data);
    std::string escape(std::string data);
    uint8_t hexDigitToDecimal(const char singleDigit);
  };
};

#endif // ALTACORE_UTIL_HPP
