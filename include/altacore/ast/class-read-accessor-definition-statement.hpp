#ifndef ALTACORE_AST_CLASS_READ_ACCESSOR_DEFNITION_STATEMENT_HPP
#define ALTACORE_AST_CLASS_READ_ACCESSOR_DEFNITION_STATEMENT_HPP

#include "class-statement-node.hpp"
#include "type.hpp"
#include "block-node.hpp"
#include "../det/scope.hpp"
#include "../det/function.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class ClassReadAccessorDefinitionStatement: public ClassStatementNode {
    public:
      virtual const NodeType nodeType();

      Visibility visibilityModifier = Visibility::Private;
      std::string name;
      std::shared_ptr<Type> type = nullptr;
      std::shared_ptr<BlockNode> body = nullptr;

      std::shared_ptr<DET::Scope> $bodyScope = nullptr;
      std::shared_ptr<DET::Function> $function = nullptr;

      ClassReadAccessorDefinitionStatement(Visibility visibilityModifier);

      virtual void detail(std::shared_ptr<DET::Scope> scope);
      ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_READ_ACCESSOR_DEFNITION_STATEMENT_HPP
