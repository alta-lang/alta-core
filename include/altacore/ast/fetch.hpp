#ifndef ALTACORE_AST_FETCH_HPP
#define ALTACORE_AST_FETCH_HPP

#include "retrieval-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"
#include "type.hpp"

namespace AltaCore {
  namespace AST {
    class Accessor; // forward declaration
    class Fetch: public RetrievalNode {
      public:
        virtual const NodeType nodeType();

        std::vector<std::shared_ptr<Type>> genericArguments;

        Fetch(std::string query);

        void narrowTo(std::shared_ptr<DH::Fetch> info, std::shared_ptr<DET::Type> type);
        void narrowTo(std::shared_ptr<DH::Fetch> info, size_t i);
        void widen(std::shared_ptr<DH::Fetch> info);
        
        ALTACORE_AST_DETAIL(Fetch);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FETCH_HPP
