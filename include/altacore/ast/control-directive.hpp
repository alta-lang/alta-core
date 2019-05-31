#ifndef ALTACORE_AST_CONTROL_DIRECTIVE_HPP
#define ALTACORE_AST_CONTROL_DIRECTIVE_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class ControlDirective: public StatementNode {
      public:
        virtual const NodeType nodeType();

        bool isBreak = false;

        ControlDirective() {};
    };
  };
};

#endif // ALTACORE_AST_CONTROL_DIRECTIVE_HPP
