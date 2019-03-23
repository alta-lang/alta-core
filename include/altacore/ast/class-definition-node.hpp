#ifndef ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
#define ALTACORE_AST_CLASS_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "class-statement-node.hpp"
#include "class-special-method-definition-statement.hpp"
#include "../det/class.hpp"
#include "retrieval-node.hpp"
#include <vector>
#include <string>
#include "generic.hpp"

namespace AltaCore {
  namespace AST {
    class ClassDefinitionNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::vector<std::shared_ptr<Generic>> generics;
        std::string name;
        std::vector<std::shared_ptr<ClassStatementNode>> statements;
        std::vector<std::shared_ptr<RetrievalNode>> parents;

        ClassDefinitionNode(std::string name);

        ALTACORE_AST_DETAIL(ClassDefinitionNode);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
