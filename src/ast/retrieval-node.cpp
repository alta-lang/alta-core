#include "../../include/altacore/ast/retrieval-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RetrievalNode::nodeType() {
  return NodeType::RetrievalNode;
};

AltaCore::AST::RetrievalNode::RetrievalNode(std::string _query):
  query(_query)
  {};
