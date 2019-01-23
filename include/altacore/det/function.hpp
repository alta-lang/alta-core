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
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        static std::shared_ptr<Function> create(std::shared_ptr<Scope> parentScope, std::string name, std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters, std::shared_ptr<Type> returnType);

        std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters;
        std::vector<std::shared_ptr<Variable>> parameterVariables;
        std::shared_ptr<Type> returnType = nullptr;
        std::shared_ptr<Scope> scope = nullptr;
        bool isLiteral = false;
        bool isExport = false;
        bool isMethod = false;

        std::shared_ptr<Type> parentClassType = nullptr;

        // external (i.e. used in the parameters or return type)
        std::vector<std::shared_ptr<Type>> publicHoistedFunctionalTypes;

        // internal (i.e. used in the body)
        std::vector<std::shared_ptr<Type>> hoistedFunctionalTypes;

        Function(std::shared_ptr<Scope> parentScope, std::string name);
    };
  };
};

#endif // ALTACORE_DET_FUNCTION_HPP
