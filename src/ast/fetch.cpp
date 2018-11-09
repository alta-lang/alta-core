#include "../include/altacore/ast/fetch.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Fetch::nodeType() {
  return NodeType::Fetch;
};

AltaCore::AST::Fetch::Fetch(std::string _query):
  query(_query)
  {};

void AltaCore::AST::Fetch::detail(AltaCore::DET::Scope* scope) {
  auto items = scope->findAll(query);

  if (items.size() == 1) {
    $item = items[0];
  } else if (items.size() > 0) {
    // TODO: do something better with multiple item results
    //       for now, just pick the first one
    $item = items[0];
  } else {
    // nothing was found for our query, throw an error.
    // TODO: throw it politely. i.e. through a logger.
    //       that, though, will have to wait until we implement
    //       a good logger for the whole core functionality (lexer, parser, AST,
    //       DET, module system, etc.). that way, we'll give users
    //       an easy to use commmon interface for handling our errors
    throw std::runtime_error("OH NO! THERE'S NO `" + query + "` IN THE SCOPE!");
  }
};