#ifndef ALTACORE_AST_CLASS_SPECIAL_METHOD_DEFNITION_STATEMENT_HPP
#define ALTACORE_AST_CLASS_SPECIAL_METHOD_DEFNITION_STATEMENT_HPP

#include "class-statement-node.hpp"
#include "parameter.hpp"
#include "block-node.hpp"
#include "../det/function.hpp"
#include "attribute-node.hpp"

namespace AltaCore {
  namespace AST {
    enum class SpecialClassMethod {
      Constructor,
      Destructor,
      From,
      To,
    };

    class ClassSpecialMethodDefinitionStatement: public ClassStatementNode, public std::enable_shared_from_this<ClassSpecialMethodDefinitionStatement> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::shared_ptr<AttributeNode>> attributes;
        Visibility visibilityModifier = Visibility::Private;
        SpecialClassMethod type = SpecialClassMethod::Constructor;
        std::vector<std::shared_ptr<Parameter>> parameters;
        std::shared_ptr<BlockNode> body = nullptr;
        std::shared_ptr<Type> specialType = nullptr;

        ClassSpecialMethodDefinitionStatement(Visibility visibilityModifier, SpecialClassMethod type);

        ALTACORE_AST_DETAIL_NO_BODY_OPT(ClassSpecialMethodDefinitionStatement);
        ALTACORE_AST_INFO_DETAIL(ClassSpecialMethodDefinitionStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CLASS_SPECIAL_METHOD_DEFNITION_STATEMENT_HPP
