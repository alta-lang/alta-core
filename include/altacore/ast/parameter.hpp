#ifndef ALTACORE_AST_PARAMETER_HPP
#define ALTACORE_AST_PARAMETER_HPP

#include "node.hpp"
#include "parameter.hpp"
#include "type.hpp"
#include "../det/scope.hpp"
#include "attribute-node.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class Parameter: public Node, public std::enable_shared_from_this<Parameter> {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::shared_ptr<Type> type = nullptr;
        bool isVariable = false;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        Parameter() {};
        Parameter(std::string name, std::shared_ptr<Type> type, bool isVariable = false);

        virtual std::shared_ptr<DH::Node> detail(std::shared_ptr<DET::Scope> scope, bool hoist = true);
        std::shared_ptr<DH::Parameter> fullDetail(std::shared_ptr<DET::Scope> scope, bool hoist = true) {
          return std::dynamic_pointer_cast<DH::Parameter>(detail(scope));
        };
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_PARAMETER_HPP
