#ifndef ALTACORE_UTIL_HPP
#define ALTACORE_UTIL_HPP

#include "ast.hpp"
#include "det.hpp"

namespace AltaCore {
  namespace Util {
    bool isInFunction(AltaCore::DET::ScopeItem* item);
    std::weak_ptr<DET::Module> getModule(AltaCore::DET::Scope* scope);
    std::string unescape(std::string data);
  };
};

#endif // ALTACORE_UTIL_HPP
