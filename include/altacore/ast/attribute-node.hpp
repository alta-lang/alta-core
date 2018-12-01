#ifndef ALTACORE_AST_ATTRIBUTE_NODE_HPP
#define ALTACORE_AST_ATTRIBUTE_NODE_HPP

#include "node.hpp"
#include <vector>
#include <string>
#include "literal-node.hpp"
#include "../attributes.hpp"
#include <optional>

namespace AltaCore {
  namespace AST {
    class AttributeNode: public Node {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> accessors;
        std::vector<std::shared_ptr<LiteralNode>> arguments;

        std::weak_ptr<DET::Module> $module;
        std::vector<Attributes::AttributeArgument> $arguments;
        std::optional<Attributes::Attribute> $attribute = std::nullopt;

        AttributeNode(std::vector<std::string> accessors, std::vector<std::shared_ptr<LiteralNode>> arguments = {});

        bool matches(std::vector<std::string> path);
        void findAttribute();
        void run(std::shared_ptr<Node> target = nullptr);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_ATTRIBUTE_NODE_HPP
