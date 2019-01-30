#include "../../include/altacore/ast/node.hpp"
#include <sstream>
#include <crossguid/guid.hpp>

AltaCore::AST::Position::Position(size_t _line, size_t _column):
  line(_line),
  column(_column)
  {};

const AltaCore::AST::NodeType AltaCore::AST::Node::nodeType() {
  return NodeType::Node;
};

void AltaCore::AST::Node::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {};

AltaCore::AST::Node::Node() {
  std::stringstream uuidStream;
  uuidStream << xg::newGuid();
  id = uuidStream.str();
};
