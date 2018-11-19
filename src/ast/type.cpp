#include "../../include/altacore/ast/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Type::nodeType() {
  return NodeType::Type;
};

AltaCore::AST::Type::Type(std::string _name, std::vector<uint8_t> _modifiers):
  name(_name),
  modifiers(_modifiers)
  {};
AltaCore::AST::Type::Type(std::shared_ptr<AltaCore::AST::Type> _returnType, std::vector<std::shared_ptr<AltaCore::AST::Type>> _parameters, std::vector<uint8_t> _modifiers):
  isFunction(true),
  returnType(_returnType),
  parameters(_parameters),
  modifiers(_modifiers)
  {};

void AltaCore::AST::Type::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  if (isFunction) {
    std::vector<std::shared_ptr<DET::Type>> detParams;
    returnType->detail(scope);
    for (auto& param: parameters) {
      param->detail(scope);
      detParams.push_back(param->$type);
    }
    $type = std::make_shared<DET::Type>(returnType->$type, detParams, modifiers);
  } else {
    DET::NativeType nt;

    if (name == "int") {
      nt = DET::NativeType::Integer;
    } else if (name == "byte" || name == "char") {
      nt = DET::NativeType::Byte;
    } else if (name == "bool") {
      nt = DET::NativeType::Bool;
    } else {
      throw std::runtime_error("non-native types aren't currently supported");
    }
    
    $type = std::make_shared<DET::Type>(nt, modifiers);
  }
};
