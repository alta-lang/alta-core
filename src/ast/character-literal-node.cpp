#include "../../include/altacore/ast/character-literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::CharacterLiteralNode::nodeType() {
  return NodeType::CharacterLiteralNode;
};

AltaCore::AST::CharacterLiteralNode::CharacterLiteralNode(char _value, bool _escaped):
  LiteralNode('\'' + std::string(_escaped ? "\\" : "") + _value + '\''),
  value(_value),
  escaped(_escaped)
  {};
