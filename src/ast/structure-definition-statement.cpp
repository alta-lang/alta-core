#include "../../include/altacore/ast/structure-definition-statement.hpp"
#include "../../include/altacore/util.hpp"
#include <sstream>
#include <crossguid/guid.hpp>

const AltaCore::AST::NodeType AltaCore::AST::StructureDefinitionStatement::nodeType() {
  return NodeType::StructureDefinitionStatement;
};

AltaCore::AST::StructureDefinitionStatement::StructureDefinitionStatement(std::string _name):
  name(_name)
  {};

ALTACORE_AST_DETAIL_D(StructureDefinitionStatement) {
  ALTACORE_MAKE_DH(StructureDefinitionStatement);

  info->structure = DET::Class::create(name, info->inputScope, {}, true);
  info->inputScope->items.push_back(info->structure);
  info->structure->isStructure = true;
  info->structure->isExternal = info->isExternal = isExternal;
  info->structure->isTyped = info->isTyped = isTyped;

  info->structure->isLiteral = info->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->structure);
    }
  }

  auto voidType = std::make_shared<DET::Type>(DET::NativeType::Void);

  info->structure->constructors.push_back(DET::Function::create(info->structure->scope, "constructor", {}, voidType));

  for (size_t i = 0; i < members.size(); i++) {
    auto& [type, name] = members[i];
    auto det = type->fullDetail(info->inputScope);
    info->memberTypes.push_back(det);
    auto var = std::make_shared<DET::Variable>(name, det->type, info->structure->scope);
    var->visibility = Visibility::Public;
    info->structure->scope->items.push_back(var);

    std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> params;
    for (size_t j = 0; j < i; j++) {
      auto& [type2, name2] = members[j];
      std::stringstream uuidStream;
      uuidStream << xg::newGuid();
      auto newID = uuidStream.str();
      params.push_back(std::make_tuple(name2, info->memberTypes[j]->type, false, newID));
    }

    std::stringstream uuidStream;
    uuidStream << xg::newGuid();
    auto newID = uuidStream.str();
    params.push_back(std::make_tuple(name, info->memberTypes[i]->type, false, newID));

    info->structure->constructors.push_back(DET::Function::create(info->structure->scope, "constructor", params, voidType));
  }

  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(StructureDefinitionStatement) {
  ALTACORE_VS_S(StructureDefinitionStatement);

  ALTACORE_VS_E;
};