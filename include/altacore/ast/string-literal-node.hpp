#ifndef ALTACORE_AST_STRING_LITERAL_NODE_HPP
#define ALTACORE_AST_STRING_LITERAL_NODE_HPP

#include "literal-node.hpp"

namespace AltaCore {
  namespace AST {
    class StringLiteralNode: public LiteralNode {
      public:
        virtual const NodeType nodeType();

        std::string value;

        StringLiteralNode(std::string value);

        ALTACORE_AST_AUTO_DETAIL(StringLiteralNode);
    };
  };
};

#endif // ALTACORE_AST_STRING_LITERAL_NODE_HPP
