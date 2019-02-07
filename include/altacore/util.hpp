#ifndef ALTACORE_UTIL_HPP
#define ALTACORE_UTIL_HPP

#include "ast.hpp"
#include "det.hpp"

namespace AltaCore {
  namespace Util {
    static const char* const hexDigits = "0123456789abcdef";
    
    bool isInFunction(const AltaCore::DET::ScopeItem* item);
    std::weak_ptr<DET::Module> getModule(AltaCore::DET::Scope* scope);
    std::weak_ptr<DET::Function> getFunction(std::shared_ptr<AltaCore::DET::Scope> scope);
    std::string unescape(const std::string& data);
    std::string escape(const std::string& data);
    uint8_t hexDigitToDecimal(const char singleDigit);
    bool stringsAreEqualCaseInsensitive(const std::string& lhs, const std::string& rhs);
  };
};

#endif // ALTACORE_UTIL_HPP
