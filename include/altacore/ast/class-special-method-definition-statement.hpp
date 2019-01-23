#ifndef ALTACORE_AST_CLASS_SPECIAL_METHOD_DEFNITION_STATEMENT_HPP
#define ALTACORE_AST_CLASS_SPECIAL_METHOD_DEFNITION_STATEMENT_HPP

#include "class-statement-node.hpp"
#include "parameter.hpp"
#include "block-node.hpp"
#include "../det/function.hpp"

namespace AltaCore {
  namespace AST {
    enum class SpecialClassMethod {
      Constructor,
      Destructor,
    };
    
    class ClassSpecialMethodDefinitionStatement: public ClassStatementNode {
      public:
        virtual const NodeType nodeType();

        Visibility visibilityModifier = Visibility::Private;
        SpecialClassMethod type = SpecialClassMethod::Constructor;
        std::vector<std::shared_ptr<Parameter>> parameters;
        std::shared_ptr<BlockNode> body = nullptr;

        std::shared_ptr<DET::Class> $klass = nullptr;
        std::shared_ptr<DET::Function> $method = nullptr;

        ClassSpecialMethodDefinitionStatement(Visibility visibilityModifier, SpecialClassMethod type);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_CLASS_SPECIAL_METHOD_DEFNITION_STATEMENT_HPP
