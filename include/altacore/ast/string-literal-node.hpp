#ifndef ALTACORE_AST_STRING_LITERAL_NODE_HPP
#define ALTACORE_AST_STRING_LITERAL_NODE_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class StringLiteralNode: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string value;

        StringLiteralNode(std::string value);
    };
  };
};

#endif // ALTACORE_AST_STRING_LITERAL_NODE_HPP
