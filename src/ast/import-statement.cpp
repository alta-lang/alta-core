#include "../../include/altacore/ast/import-statement.hpp"
#include "../../include/altacore/modules.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ImportStatement::nodeType() {
  return NodeType::ImportStatement;
};

AltaCore::AST::ImportStatement::ImportStatement(std::string _request, std::vector<std::pair<std::string, std::string>> _imports):
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
    auto ns = std::make_shared<DET::Namespace>(alias, $parentModule->scope);
    ns->scope = $importedModule->exports;
    $parentModule->scope->items.push_back(ns);
  } else {
    for (auto& [imp, alias]: imports) {
      auto items = $importedModule->exports->findAll(imp);
      $importedItems.insert($importedItems.end(), items.begin(), items.end());
      if (!alias.empty()) {
        for (auto& item: items) {
          auto aliasItem = std::make_shared<DET::Alias>(alias, item, $parentModule->scope);
          $parentModule->scope->items.push_back(aliasItem);
        }
      } else {
        $parentModule->scope->items.insert($parentModule->scope->items.end(), items.begin(), items.end());
      }
    }
  }
};

ALTACORE_AST_VALIDATE_D(ImportStatement) {
  ALTACORE_VS_S;
  if (request.empty()) throw ValidationError("empty request/query string for import statement");
  if (isAliased) {
    if (imports.size() > 0) throw ValidationError("no individual imports should be present for aliased imports");
    if (alias.empty()) throw ValidationError("empty alias for aliased import");
  } else {
    if (!alias.empty()) throw ValidationError("no aliases should be present for non-alised imports");
    for (auto& [imp, alias]: imports) {
      if (imp.empty()) throw ValidationError("empty individual import for non-alised import");
    }
  }
  if (!$importedAST) throw ValidationError("improperly detailed import statement: empty AST");
  $importedAST->validate(stack);
  // TODO: validate the detailed information
  //       i'm too lazy right now
  ALTACORE_VS_E;
};