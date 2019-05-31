#ifndef ALTACORE_AST_CLASS_INSTANTIATION_EXPRESSION_HPP
#define ALTACORE_AST_CLASS_INSTANTIATION_EXPRESSION_HPP

#include "expression-node.hpp"
#include "../det/class.hpp"
#include <string>
#include "../simple-map.hpp"
#include "../variant.hpp"

namespace AltaCore {
  namespace AST {
    class ClassInstantiationExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        // this says that it contains an ExpressionNode,
        // but in reality it can only contain a Fetch or an Accessor
        // don't try to give it anything else
        // TODO: make Fetch and Accessor inherit from a shared base class,
        //       something specifically for scope retrieval classes
        std::shared_ptr<ExpressionNode> target;
        std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> arguments;
        bool persistent = false;

        ClassInstantiationExpression() {};

        ALTACORE_AST_DETAIL(ClassInstantiationExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_INSTANTIATION_EXPRESSION_HPP
