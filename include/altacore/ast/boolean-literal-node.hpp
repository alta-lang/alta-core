#ifndef ALTACORE_AST_BOOLEAN_LITERAL_NODE_HPP
#define ALTACORE_AST_BOOLEAN_LITERAL_NODE_HPP

#include "literal-node.hpp"

namespace AltaCore {
  namespace AST {
    class BooleanLiteralNode: public LiteralNode {
      public:
        virtual const NodeType nodeType();

        bool value;

        BooleanLiteralNode(bool value);

        ALTACORE_AST_AUTO_DETAIL(BooleanLiteralNode);
    };
  };
};

#endif // ALTACORE_AST_BOOLEAN_LITERAL_NODE_HPP
