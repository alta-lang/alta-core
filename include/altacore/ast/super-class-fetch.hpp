#ifndef ALTACORE_AST_SUPER_CLASS_FETCH_HPP
#define ALTACORE_AST_SUPER_CLASS_FETCH_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"

namespace AltaCore {
  namespace AST {
    class SuperClassFetch: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> fetch = nullptr;
        //std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> arguments;

        SuperClassFetch() {};
        
        ALTACORE_AST_DETAIL(SuperClassFetch);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif /* ALTACORE_AST_SUPER_CLASS_FETCH_HPP */
