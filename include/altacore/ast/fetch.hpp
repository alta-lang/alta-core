#ifndef ALTACORE_AST_FETCH_HPP
#define ALTACORE_AST_FETCH_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"

namespace AltaCore {
  namespace AST {
    class Fetch: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string query;

        Fetch(std::string query);

        void narrowTo(std::shared_ptr<DH::Fetch> info, std::shared_ptr<DET::Type> type);
        
        ALTACORE_AST_DETAIL(Fetch);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FETCH_HPP
