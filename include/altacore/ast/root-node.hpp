#ifndef ALTACORE_AST_ROOT_NODE_HPP
#define ALTACORE_AST_ROOT_NODE_HPP

#include "node.hpp"
#include "statement-node.hpp"
#include "../det/module.hpp"
#include "../det/scope.hpp"
#include <vector>
#include "../fs.hpp"

namespace AltaCore {
  namespace AST {
    class RootNode: public Node {
      public:
        virtual const NodeType nodeType();

        std::vector<StatementNode*> statements;

        DET::Module* $module;
        RootNode* parent = nullptr;

        RootNode();
        RootNode(std::vector<StatementNode*> statements);

        void detail(Filesystem::Path filePath, std::string moduleName = "");
        void detail(std::string filePath, std::string moduleName = "");
    };
  };
};

#endif // ALTACORE_AST_ROOT_NODE_HPP