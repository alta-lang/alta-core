#ifndef ALTACORE_DET_FUNCTION_HPP
#define ALTACORE_DET_FUNCTION_HPP

#include "scope-item.hpp"
#include "type.hpp"
#include "variable.hpp"
#include <string>
#include <vector>
#include <tuple>
#include "../event-manager.hpp"

namespace AltaCore {
  void registerGlobalAttributes();

  namespace AST {
    class FunctionDefinitionNode;
  };
  namespace DetailHandles {
    class FunctionDefinitionNode;
  };
  namespace DET {
    class Function: public ScopeItem, public std::enable_shared_from_this<Function> {
      friend void AltaCore::registerGlobalAttributes();

      private:
        bool _throws = false;
        bool _virtual = false;
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
        bool isAccessor = false;
        bool isDestructor = false;
        bool isLambda = false;
        bool isOperator = false;
        bool isGenerator = false;
        Shared::ClassOperatorType operatorType = Shared::ClassOperatorType::NONE;
        Shared::ClassOperatorOrientation orientation = Shared::ClassOperatorOrientation::Unary;
        std::shared_ptr<Type> generatorParameterType = nullptr;
        std::shared_ptr<Type> generatorReturnType = nullptr;

        // only used for lambdas
        std::vector<std::shared_ptr<Variable>> referencedVariables;

        bool throws() const {
          return _throws;
        };
        void throws(bool value) {
          _throws = value;
          if (value) beganThrowing.dispatch();
        };

        std::shared_ptr<Type> parentClassType = nullptr;

        std::vector<std::shared_ptr<Type>> genericArguments;

        std::weak_ptr<AST::FunctionDefinitionNode> ast;
        std::weak_ptr<DetailHandles::FunctionDefinitionNode> info;

        EventManager<true> beganThrowing;
        EventManager<true> doneDetailing;

        Function(std::shared_ptr<Scope> parentScope, std::string name);

        std::shared_ptr<Function> instantiateGeneric(std::vector<std::shared_ptr<Type>> genericArguments);

        void recreate(std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters, std::shared_ptr<Type> returnType);

        bool isVirtual();

        void setVirtual(bool virt) {
          _virtual = virt;
        };

        virtual std::string toString() const;
        virtual std::vector<std::shared_ptr<ScopeItem>> fullPrivateHoistedItems() const;
    };
  };
};

#endif // ALTACORE_DET_FUNCTION_HPP
