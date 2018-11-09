#ifndef ALTACORE_AST_INTEGER_LITERAL_NODE_HPP
#define ALTACORE_AST_INTEGER_LITERAL_NODE_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class IntegerLiteralNode: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string raw;

        IntegerLiteralNode(std::string raw);
    };
  };
};

#endif // ALTACORE_AST_INTEGER_LITERAL_NODE_HPP