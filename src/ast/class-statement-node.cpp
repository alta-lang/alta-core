#include "../../include/altacore/ast/class-statement-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassStatementNode::nodeType() {
  return NodeType::ClassStatementNode;
};

AltaCore::AST::Visibility AltaCore::AST::parseVisibility(std::string visibilityString) {
  if (visibilityString == "private") {
    return Visibility::Private;
  }
  if (visibilityString == "protected") {
    return Visibility::Protected;
  }
  if (visibilityString == "public") {
    return Visibility::Public;
  }
  throw std::runtime_error("failed to parse visibility (input was \"" + visibilityString + "\")");
};
