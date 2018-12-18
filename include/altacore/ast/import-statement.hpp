#ifndef ALTACORE_AST_IMPORT_STATEMENT_HPP
#define ALTACORE_AST_IMPORT_STATEMENT_HPP

#include <utility>
#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"
#include "../det/module.hpp"
#include "root-node.hpp"

namespace AltaCore {
  namespace AST {
    class ImportStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::string request;
        bool isAliased = false;
        std::vector<std::pair<std::string, std::string>> imports; // only available on non-alias imports
        std::string alias;                                        // only available on alias imports

        std::shared_ptr<DET::Module> $parentModule = nullptr;
        std::shared_ptr<DET::Module> $importedModule = nullptr;
        std::shared_ptr<AST::RootNode> $importedAST = nullptr;
        std::vector<std::shared_ptr<DET::ScopeItem>> $importedItems;

        ImportStatement() {};
        ImportStatement(std::string request, std::vector<std::pair<std::string, std::string>> imports);
        ImportStatement(std::string request, std::string alias);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_IMPORT_STATEMENT_HPP
