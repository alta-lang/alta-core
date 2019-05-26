#include "../../include/altacore/ast/export-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ExportStatement::nodeType() {
  return NodeType::ExportStatement;
};

ALTACORE_AST_DETAIL_D(ExportStatement) {
  ALTACORE_MAKE_DH(ExportStatement);

  if (externalTarget) {
    externalTarget->isManual = true;
    info->externalTarget = externalTarget->fullDetail(info->inputScope);
    if (externalTarget->isAliased) {
      if (externalTarget->alias.empty()) {
        info->externalTarget->parentModule->exports->items.insert(
          info->externalTarget->parentModule->exports->items.end(),
          info->externalTarget->importedModule->exports->items.begin(),
          info->externalTarget->importedModule->exports->items.end()
        );
      } else {
        auto ns = std::make_shared<DET::Namespace>(externalTarget->alias, info->inputScope);
        ns->scope = info->externalTarget->importedModule->exports;
        info->externalTarget->parentModule->exports->items.push_back(ns);
      }
    } else {
      for (auto [imp, alias]: externalTarget->imports) {
        auto items = info->externalTarget->importedModule->exports->findAll(imp);
        info->externalTarget->importedItems.insert(info->externalTarget->importedItems.end(), items.begin(), items.end());
        for (auto& item: items) {
          auto aliasItem = std::make_shared<DET::Alias>(alias.empty() ? imp : alias, item, info->externalTarget->parentModule->exports);
          info->externalTarget->parentModule->exports->items.push_back(aliasItem);
        }
      }
    }
  } else {
    info->localTarget = localTarget->fullDetail(info->inputScope);

    auto parentMod = Util::getModule(info->inputScope.get()).lock();

    for (auto& item: info->localTarget->items) {
      if (auto var = std::dynamic_pointer_cast<DET::Variable>(item)) {
        var->isExport = true;
      } else if (auto func = std::dynamic_pointer_cast<DET::Function>(item)) {
        func->isExport = true;
      } else if (auto klass = std::dynamic_pointer_cast<DET::Class>(item)) {
        klass->isExport = true;
      }

      auto aliasItem = std::make_shared<DET::Alias>(localTargetAlias.empty() ? localTarget->query : localTargetAlias, item, parentMod->exports);
      parentMod->exports->items.push_back(aliasItem);
    }
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(ExportStatement) {
  ALTACORE_VS_S(ExportStatement);

  if (externalTarget) {
    externalTarget->validate(stack, info->externalTarget);
  } else if (localTarget) {
    localTarget->validate(stack, info->localTarget);
  } else {
    ALTACORE_VALIDATION_ERROR("export statement must have (a) local or external target[s]");
  }

  ALTACORE_VS_E;
};
