#ifndef ALTACORE_AST_CLASS_MEMBER_DEFNITION_STATEMENT_HPP
#define ALTACORE_AST_CLASS_MEMBER_DEFNITION_STATEMENT_HPP

#include "class-statement-node.hpp"
#include "variable-definition-expression.hpp"

namespace AltaCore {
  namespace AST {
    class ClassMemberDefinitionStatement: public ClassStatementNode {
      public:
        virtual const NodeType nodeType();

        Visibility visibilityModifier = Visibility::Private;
        std::shared_ptr<VariableDefinitionExpression> varDef = nullptr;

        ClassMemberDefinitionStatement(Visibility visibilityModifier);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_MEMBER_DEFNITION_STATEMENT_HPP
