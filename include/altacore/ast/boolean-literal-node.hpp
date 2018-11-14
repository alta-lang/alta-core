#ifndef ALTACORE_AST_BOOLEAN_LITERAL_NODE_HPP
#define ALTACORE_AST_BOOLEAN_LITERAL_NODE_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class BooleanLiteralNode: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        bool value;

        BooleanLiteralNode(bool value);
    };
  };
};

#endif // ALTACORE_AST_BOOLEAN_LITERAL_NODE_HPP
