#ifndef ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
#define ALTACORE_AST_CLASS_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "class-statement-node.hpp"
#include "class-special-method-definition-statement.hpp"
#include "../det/class.hpp"
#include <vector>
#include <string>

namespace AltaCore {
  namespace AST {
    class ClassDefinitionNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::string name;
        std::vector<std::shared_ptr<ClassStatementNode>> statements;

        bool isExport = false;
        bool isLiteral = false;
        bool $createDefaultConstructor = false;
        std::shared_ptr<ClassSpecialMethodDefinitionStatement> $defaultConstructor = nullptr;
        std::shared_ptr<DET::Class> $klass = nullptr;

        ClassDefinitionNode(std::string name);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
