#ifndef ALTACORE_AST_ATTRIBUTE_NODE_HPP
#define ALTACORE_AST_ATTRIBUTE_NODE_HPP

#if defined(__has_include) && __has_include(<optional>)
#include <optional>
#define ALTACORE_OPTIONAL std::optional
#define ALTACORE_NULLOPT std::nullopt
#else
#include <experimental/optional>
#define ALTACORE_OPTIONAL std::experimental::optional
#define ALTACORE_NULLOPT std::experimental::nullopt
#endif

#include "node.hpp"
#include <vector>
#include <string>
#include "literal-node.hpp"
#include "../attributes.hpp"

namespace AltaCore {
  namespace AST {
    class AttributeNode: public Node {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> accessors;
        std::vector<std::shared_ptr<LiteralNode>> arguments;

        std::weak_ptr<DET::Module> $module;
        std::vector<Attributes::AttributeArgument> $arguments;
        ALTACORE_OPTIONAL<Attributes::Attribute> $attribute = ALTACORE_NULLOPT;

        AttributeNode(std::vector<std::string> accessors, std::vector<std::shared_ptr<LiteralNode>> arguments = {});

        bool matches(std::vector<std::string> path);
        void findAttribute();
        void run(std::shared_ptr<Node> target = nullptr);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_ATTRIBUTE_NODE_HPP
