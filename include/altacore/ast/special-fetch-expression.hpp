#ifndef ALTACORE_AST_SPECIAL_FETCH_EXPRESSION_HPP
#define ALTACORE_AST_SPECIAL_FETCH_EXPRESSION_HPP

#include "retrieval-node.hpp"

namespace AltaCore {
  namespace AST {
    class SpecialFetchExpression: public RetrievalNode {
      public:
        virtual const NodeType nodeType();

        SpecialFetchExpression():
          RetrievalNode("$")
          {};

        ALTACORE_AST_DETAIL(SpecialFetchExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_SPECIAL_FETCH_EXPRESSION_HPP
