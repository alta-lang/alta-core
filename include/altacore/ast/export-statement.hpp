#ifndef ALTACORE_AST_EXPORT_STATEMENT_HPP
#define ALTACORE_AST_EXPORT_STATEMENT_HPP

#include "statement-node.hpp"
#include "retrieval-node.hpp"
#include "import-statement.hpp"

namespace AltaCore {
  namespace AST {
    class ExportStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::vector<std::pair<std::shared_ptr<RetrievalNode>, std::string>> localTargets;
        std::shared_ptr<ImportStatement> externalTarget = nullptr;

        ExportStatement() {};

        ALTACORE_AST_DETAIL(ExportStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_EXPORT_STATEMENT_HPP
