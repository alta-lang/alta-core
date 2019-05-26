#ifndef ALTACORE_AST_SIZEOF_OPERATION_HPP
#define ALTACORE_AST_SIZEOF_OPERATION_HPP

#include "type.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class SizeofOperation: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<Type> target = nullptr;

        SizeofOperation() {};

        ALTACORE_AST_DETAIL(SizeofOperation);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_SIZEOF_OPERATION_HPP
