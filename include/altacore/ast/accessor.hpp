#ifndef ALTACORE_AST_ACCESSOR_HPP
#define ALTACORE_AST_ACCESSOR_HPP

#include "retrieval-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/function.hpp"

namespace AltaCore {
  namespace AST {
    class Accessor: public RetrievalNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target = nullptr;

        Accessor(std::shared_ptr<AST::ExpressionNode> target, std::string query);

        void narrowTo(std::shared_ptr<DH::Accessor> info, std::shared_ptr<DET::Type> type);
        
        ALTACORE_AST_DETAIL(Accessor);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ACCESSOR_HPP
