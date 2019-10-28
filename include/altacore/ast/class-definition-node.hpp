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

// DO NOT TOUCH, internal use only
namespace AltaCoreClassHelpers {
  template<class T>
  void detailClass(std::shared_ptr<T> info, AltaCore::AST::ClassDefinitionNode* self);
};

namespace AltaCore {
  namespace AST {
    class ClassDefinitionNode: public StatementNode, public std::enable_shared_from_this<ClassDefinitionNode> {
      template<class T>
      friend void AltaCoreClassHelpers::detailClass(std::shared_ptr<T> info, AltaCore::AST::ClassDefinitionNode* self);

      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::vector<std::shared_ptr<Generic>> generics;
        std::string name;
        std::vector<std::shared_ptr<ClassStatementNode>> statements;
        std::vector<std::shared_ptr<RetrievalNode>> parents;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        ClassDefinitionNode(std::string name);

        ALTACORE_AST_DETAIL(ClassDefinitionNode);
        ALTACORE_AST_VALIDATE;

        std::shared_ptr<DET::Class> instantiateGeneric(std::shared_ptr<DH::ClassDefinitionNode> info, std::vector<std::shared_ptr<DET::Type>> genericArguments);
    };
  };
};

#endif // ALTACORE_AST_CLASS_DEFINITION_NODE_HPP
