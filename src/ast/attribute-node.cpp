#include "../../include/altacore/ast/attribute-node.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AttributeNode::nodeType() {
  return NodeType::AttributeNode;
};

AltaCore::AST::AttributeNode::AttributeNode(std::vector<std::string> _accessors, std::vector<std::shared_ptr<AltaCore::AST::LiteralNode>> _arguments):
  accessors(_accessors),
  arguments(_arguments)
  {};

bool AltaCore::AST::AttributeNode::matches(std::vector<std::string> path) {
  if (accessors.size() != path.size()) return false;
  for (size_t i = 0; i < path.size(); i++) {
    if (accessors[i] != path[i]) return false;
  }
  return true;
};

void AltaCore::AST::AttributeNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  $module = Util::getModule(scope.get());

  for (auto& arg: arguments) {
    arg->detail(scope);
    if (arg->nodeType() == NodeType::StringLiteralNode) {
      $arguments.emplace_back(std::dynamic_pointer_cast<AST::StringLiteralNode>(arg)->value);
    } else if (arg->nodeType() == NodeType::IntegerLiteralNode) {
      $arguments.emplace_back(std::stoi(std::dynamic_pointer_cast<AST::IntegerLiteralNode>(arg)->raw));
    } else if (arg->nodeType() == NodeType::BooleanLiteralNode) {
      $arguments.emplace_back(std::dynamic_pointer_cast<AST::BooleanLiteralNode>(arg)->value);
    } else {
      throw std::runtime_error("welp.");
    }
  }

  findAttribute();
  run(target);
};

void AltaCore::AST::AttributeNode::run(std::shared_ptr<AltaCore::AST::Node> target) {
  if ($attribute) {
    auto attr = *$attribute;
    if (attr.callback) {
      attr.callback(target ? target : $module.lock()->ast.lock(), $arguments);
    }
  }
};

void AltaCore::AST::AttributeNode::findAttribute() {
  $attribute = Attributes::findAttribute(accessors, ALTACORE_NULLOPT, $module.lock()->path.toString());
};

std::string AltaCore::AST::AttributeNode::id() const {
  std::string result = "";

  bool isFirst = true;
  for (auto& item: accessors) {
    if (isFirst) {
      isFirst = false;
    } else {
      result += '.';
    }
    result += item;
  }

  return result;
};
