#ifndef ALTACORE_AST_RETRIEVAL_NODE_HPP
#define ALTACORE_AST_RETRIEVAL_NODE_HPP

#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class RetrievalNode: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string query;

        RetrievalNode(std::string query);

        ALTACORE_AST_AUTO_DETAIL(RetrievalNode);
    };
  };
};

#endif /* ALTACORE_AST_RETRIEVAL_NODE_HPP */
