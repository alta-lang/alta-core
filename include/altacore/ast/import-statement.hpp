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

        ImportStatement() {};
        ImportStatement(std::string request, std::vector<std::pair<std::string, std::string>> imports);
        ImportStatement(std::string request, std::string alias);

        ALTACORE_AST_DETAIL(ImportStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_IMPORT_STATEMENT_HPP
