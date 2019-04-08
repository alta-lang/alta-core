#ifndef ALTACORE_AST_GENERIC_HPP
#define ALTACORE_AST_GENERIC_HPP

#include "node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class Generic: public Node {
      public:
        virtual const NodeType nodeType();

        std::string name;

        Generic(std::string _name):
          name(_name)
          {};

        ALTACORE_AST_DETAIL(Generic);
    };
  };
};

#endif /* ALTACORE_AST_GENERIC_HPP */
