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

void AltaCore::AST::ImportStatement::parse(Filesystem::Path sourcePath) {
  Modules::parseModule(request, sourcePath);
};

ALTACORE_AST_DETAIL_D(ImportStatement) {
  ALTACORE_MAKE_DH(ImportStatement);
  info->parentModule = Util::getModule(scope.get()).lock();
  info->importedAST = Modules::parseModule(request, info->parentModule->path);
  info->importedAST->detail(Modules::resolve(request, info->parentModule->path));
  info->importedModule = info->importedAST->info->module;
  info->parentModule->dependencies.push_back(info->importedModule);
  info->importedModule->dependents.push_back(info->parentModule);
  if (!isManual) {
    if (isAliased) {
      auto ns = std::make_shared<DET::Namespace>(alias, info->parentModule->scope);
      ns->scope = info->importedModule->exports;
      info->parentModule->scope->items.push_back(ns);
    } else {
      for (auto& [imp, alias]: imports) {
        auto items = info->importedModule->exports->findAll(imp);
        info->importedItems.insert(info->importedItems.end(), items.begin(), items.end());
        for (auto& item: items) {
          auto aliasItem = std::make_shared<DET::Alias>(alias.empty() ? imp : alias, item, info->parentModule->scope);
          info->parentModule->scope->items.push_back(aliasItem);
        }
      }
    }
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(ImportStatement) {
  ALTACORE_VS_S(ImportStatement);
  if (request.empty()) ALTACORE_VALIDATION_ERROR("empty request/query string for import statement");
  if (!isManual) {
    if (isAliased) {
      if (imports.size() > 0) ALTACORE_VALIDATION_ERROR("no individual imports should be present for aliased imports");
      if (alias.empty()) ALTACORE_VALIDATION_ERROR("empty alias for aliased import");
    } else {
      if (!alias.empty()) ALTACORE_VALIDATION_ERROR("no aliases should be present for non-alised imports");
      for (auto& [imp, alias]: imports) {
        if (imp.empty()) ALTACORE_VALIDATION_ERROR("empty individual import for non-alised import");
      }
    }
  }
  if (!info->importedAST) ALTACORE_VALIDATION_ERROR("improperly detailed import statement: empty AST");
  info->importedAST->validate(stack, nullptr);
  // TODO: validate the detailed information
  //       i'm too lazy right now
  ALTACORE_VS_E;
};
