#ifndef ALTACORE_AST_FETCH_HPP
#define ALTACORE_AST_FETCH_HPP

#include "retrieval-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"

namespace AltaCore {
  namespace AST {
    class Fetch: public RetrievalNode {
      public:
        virtual const NodeType nodeType();

        Fetch(std::string query);

        void narrowTo(std::shared_ptr<DH::Fetch> info, std::shared_ptr<DET::Type> type);
        
        ALTACORE_AST_DETAIL(Fetch);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FETCH_HPP
