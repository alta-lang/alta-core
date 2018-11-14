#include "../../include/altacore/ast/import-statement.hpp"
#include "../../include/altacore/modules.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ImportStatement::nodeType() {
  return NodeType::ImportStatement;
};

AltaCore::AST::ImportStatement::ImportStatement(std::string _request, std::vector<std::string> _imports):
  isAliased(false),
  request(_request),
  imports(_imports)
  {};
AltaCore::AST::ImportStatement::ImportStatement(std::string _request, std::string _alias):
  isAliased(true),
  request(_request),
  alias(_alias)
  {};

void AltaCore::AST::ImportStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  $parentModule = Util::getModule(scope.get()).lock();
  $importedAST = Modules::parseModule(request, $parentModule->path);
  $importedModule = $importedAST->$module;
  $parentModule->dependencies.push_back($importedModule);
  $importedModule->dependents.push_back($parentModule);
  if (isAliased) {
    // TODO
    throw std::runtime_error("can't do aliased imports yet");
  } else {
    for (auto& imp: imports) {
      auto items = $importedModule->exports->findAll(imp);
      $importedItems.insert($importedItems.end(), items.begin(), items.end());
      $parentModule->scope->items.insert($parentModule->scope->items.end(), items.begin(), items.end());
    }
  }
};
