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

        ClassDefinitionNode(std::string name);

        ALTACORE_AST_DETAIL(ClassDefinitionNode);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
