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
    class RootNode: public Node, public std::enable_shared_from_this<RootNode> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::shared_ptr<StatementNode>> statements;

        std::shared_ptr<DH::RootNode> info;

        RootNode();
        RootNode(std::vector<std::shared_ptr<StatementNode>> statements);

        void detail(Filesystem::Path filePath, std::string moduleName = "");
        void detail(std::string filePath, std::string moduleName = "");
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ROOT_NODE_HPP
