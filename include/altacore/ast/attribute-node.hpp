#ifndef ALTACORE_AST_ATTRIBUTE_NODE_HPP
#define ALTACORE_AST_ATTRIBUTE_NODE_HPP

#include "../optional.hpp"
#include "node.hpp"
#include <vector>
#include <string>
#include "../attributes.hpp"

namespace AltaCore {
  namespace AST {
    class AttributeNode: public Node {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> accessors;
        std::vector<std::shared_ptr<Node>> arguments;

        AttributeNode() {};
        AttributeNode(std::vector<std::string> accessors, std::vector<std::shared_ptr<Node>> arguments = {});

        bool matches(std::vector<std::string> path);
        void findAttribute(std::shared_ptr<DH::AttributeNode> info);
        void run(std::shared_ptr<DH::AttributeNode> info, std::shared_ptr<Node> target = nullptr, std::shared_ptr<DH::Node> = nullptr);

        std::string id() const;

        virtual std::shared_ptr<AltaCore::DH::Node> detail(std::shared_ptr<AltaCore::DET::Scope> scope, std::shared_ptr<Node> target = nullptr, std::shared_ptr<DH::Node> info = nullptr);
          std::shared_ptr<AltaCore::DH::AttributeNode> fullDetail(std::shared_ptr<AltaCore::DET::Scope> scope, std::shared_ptr<Node> target = nullptr, std::shared_ptr<DH::Node> info = nullptr) {
          return std::dynamic_pointer_cast<AltaCore::DH::AttributeNode>(detail(scope, target, info));
        };
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ATTRIBUTE_NODE_HPP
