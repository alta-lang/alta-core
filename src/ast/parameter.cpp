#include "../../include/altacore/ast/parameter.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Parameter::nodeType() {
  return NodeType::Parameter;
};

AltaCore::AST::Parameter::Parameter(std::string _name, std::shared_ptr<AltaCore::AST::Type> _type):
  name(_name),
  type(_type)
  {};

void AltaCore::AST::Parameter::detail(std::shared_ptr<DET::Scope> scope) {
  return type->detail(scope);
};
