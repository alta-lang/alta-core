#ifndef ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
#define ALTACORE_AST_CLASS_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "class-statement-node.hpp"
#include <vector>
#include <string>

namespace AltaCore {
  namespace AST {
    class ClassDefinitionNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::vector<std::shared_ptr<ClassStatementNode>> statements;

        ClassDefinitionNode(std::string name);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
