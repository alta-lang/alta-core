#ifndef ALTACORE_DET_VARIABLE_HPP
#define ALTACORE_DET_VARIABLE_HPP

#include "scope-item.hpp"
#include "type.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class Variable: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual Variable* clone();
        virtual Variable* deepClone();

        Type* type;
        bool isLiteral = false;

        Variable(std::string name, Type* type, Scope* parentScope = nullptr);
    };
  };
};

#endif // ALTACORE_DET_VARIABLE_HPP