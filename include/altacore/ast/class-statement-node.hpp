#ifndef ALTACORE_AST_CLASS_STATEMENT_NODE_HPP
#define ALTACORE_AST_CLASS_STATEMENT_NODE_HPP

#include "node.hpp"

namespace AltaCore {
  namespace AST {
    Visibility parseVisibility(std::string visibilityString);

    class ClassStatementNode: public Node {
      public:
        virtual const NodeType nodeType();

        ALTACORE_AST_AUTO_NO_BODY_OPT_DETAIL(ClassStatementNode);
        ALTACORE_AST_AUTO_INFO_DETAIL(ClassStatementNode);
    };
  };
};

#endif // ALTACORE_AST_CLASS_STATEMENT_NODE_HPP
