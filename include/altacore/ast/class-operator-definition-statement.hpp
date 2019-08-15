#ifndef ALTACORE_AST_CLASS_OPERATOR_DEFNITION_STATEMENT_HPP
#define ALTACORE_AST_CLASS_OPERATOR_DEFNITION_STATEMENT_HPP

#include "class-statement-node.hpp"
#include "block-node.hpp"
#include "type.hpp"

namespace AltaCore {
  namespace AST {
    enum class ClassOperatorType {
      NONE,
      Not,
      Dereference,
      Reference,
      Addition,
      Subtraction,
      Multiplication,
      Division,
      Xor,
      LeftShift,
      RightShift,
      BitAnd,
      BitOr,
      BitNot,
      And,
      Or,
      Equality,
      Inequality,
      LessThan,
      GreaterThan,
      LessThanOrEqualTo,
      GreaterThanOrEqualTo,
      Index,
    };
    enum class ClassOperatorOrientation {
      // indicates the operator takes a single argument
      Unary,
      // indicates the operator's `this` argument is on the left of the symbol
      Left,
      // indicates the operator's `this` argument is on the right of the symbol
      Right,
    };

    class ClassOperatorDefinitionStatement: public ClassStatementNode {
      public:
        virtual const NodeType nodeType();

        Visibility visibilityModifier = Visibility::Private;
        ClassOperatorType type = ClassOperatorType::NONE;
        ClassOperatorOrientation orientation = ClassOperatorOrientation::Unary;

        std::shared_ptr<BlockNode> block = nullptr;
        std::shared_ptr<Type> returnType = nullptr;
        std::shared_ptr<Type> argumentType = nullptr;

        ClassOperatorDefinitionStatement(Visibility visibilityModifier);

        ALTACORE_AST_DETAIL_NO_BODY_OPT(ClassOperatorDefinitionStatement);
        ALTACORE_AST_INFO_DETAIL(ClassOperatorDefinitionStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_OPERATOR_DEFNITION_STATEMENT_HPP
