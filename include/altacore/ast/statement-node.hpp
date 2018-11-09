#ifndef ALTACORE_AST_STATEMENT_NODE_HPP
#define ALTACORE_AST_STATEMENT_NODE_HPP

#include "node.hpp"

namespace AltaCore {
  namespace AST {
    class StatementNode: public Node {
      public:
        virtual const NodeType nodeType();
    };
  };
};

#endif // ALTACORE_AST_STATEMENT_NODE_HPP