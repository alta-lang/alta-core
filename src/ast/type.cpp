#include "../../include/altacore/ast/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Type::nodeType() {
  return NodeType::Type;
};

AltaCore::AST::Type::Type(std::string _name, std::vector<uint8_t> _modifiers):
  name(_name),
  modifiers(_modifiers)
  {};

void AltaCore::AST::Type::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  DET::NativeType nt;

  if (name == "int") {
    nt = DET::NativeType::Integer;
  } else if (name == "byte" || name == "char") {
    nt = DET::NativeType::Byte;
  } else {
    throw std::runtime_error("non-native types aren't currently supported");
  }
  
  $type = std::make_shared<DET::Type>(nt, modifiers);
};
