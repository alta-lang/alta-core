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

        ClassMethodDefinitionStatement(Visibility visibilityModifier);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_CLASS_METHOD_DEFNITION_STATEMENT_HPP
