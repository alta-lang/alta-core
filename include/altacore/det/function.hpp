#ifndef ALTACORE_DET_FUNCTION_HPP
#define ALTACORE_DET_FUNCTION_HPP

#include "scope-item.hpp"
#include "type.hpp"
#include "variable.hpp"
#include <string>
#include <vector>
#include <tuple>

namespace AltaCore {
  namespace DET {
    class Function: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual Function* clone();
        virtual Function* deepClone();

        std::vector<std::tuple<std::string, Type*>> parameters;
        std::vector<Variable*> parameterVariables;
        Type* returnType;
        Scope* scope;
        bool isLiteral;

        Function(Scope* parentScope, std::string name, std::vector<std::tuple<std::string, Type*>> parameters, Type* returnType);
    };
  };
};

#endif // ALTACORE_DET_FUNCTION_HPP