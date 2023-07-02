#ifndef ALTACORE_AST_CLASS_METHOD_DEFNITION_STATEMENT_HPP
#define ALTACORE_AST_CLASS_METHOD_DEFNITION_STATEMENT_HPP

#include "class-statement-node.hpp"
#include "function-definition-node.hpp"

namespace AltaCore {
  namespace AST {
    class ClassMethodDefinitionStatement: public ClassStatementNode {
      public:
        virtual const NodeType nodeType();

        Visibility visibilityModifier = Visibility::Private;
        std::shared_ptr<FunctionDefinitionNode> funcDef = nullptr;
        bool isStatic = false;

        ClassMethodDefinitionStatement(Visibility visibilityModifier);

        ALTACORE_AST_DETAIL_NO_BODY_OPT(ClassMethodDefinitionStatement);
        ALTACORE_AST_INFO_DETAIL(ClassMethodDefinitionStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_METHOD_DEFNITION_STATEMENT_HPP
