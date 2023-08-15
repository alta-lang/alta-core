#include "../../include/altacore/ast/root-node.hpp"
#include "../../include/altacore/modules.hpp"
#include "../../include/altacore/ast/import-statement.hpp"
#include "../../include/altacore/ast/export-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RootNode::nodeType() {
  return NodeType::RootNode;
};

AltaCore::AST::RootNode::RootNode() {};
AltaCore::AST::RootNode::RootNode(std::vector<std::shared_ptr<AltaCore::AST::StatementNode>> _statements):
  statements(_statements)
  {};

void AltaCore::AST::RootNode::detail(AltaCore::Filesystem::Path filePath, std::string moduleName, std::shared_ptr<DET::Module> parentModule) {
  if (info) return;
  info = std::make_shared<DH::RootNode>();
  
  Modules::PackageInfo pkgInfo;
  if (moduleName == "") {
    // attempt to find the module name
    try {
      pkgInfo = Modules::getInfo(filePath);
      auto relativeFilePath = filePath.relativeTo(pkgInfo.root);
      moduleName = pkgInfo.name + '/' + (relativeFilePath.dirname() / relativeFilePath.filename()).toString("/");
    } catch (const Modules::ModuleError&) {
      moduleName = filePath.filename() + "/" + filePath.filename();
      pkgInfo.name = filePath.filename();
      pkgInfo.main = filePath;
      pkgInfo.root = filePath.dirname();
      pkgInfo.outputBinary = Modules::OutputBinaryType::Exectuable;
    }
  } else {
    try {
      pkgInfo = Modules::getInfo(filePath);
    } catch (...) {
      pkgInfo.main = filePath;
      pkgInfo.name = moduleName;
      pkgInfo.root = filePath.dirname();
      pkgInfo.outputBinary = Modules::OutputBinaryType::Exectuable;
    }
  }
  info->module = DET::Module::create(moduleName, pkgInfo, filePath);
  info->module->ast = shared_from_this();

  info->module->parentModule = parentModule;

  // TODO: be smarter about this; only insert it when we actually use coroutines
  auto detailInternal = [&]() {
    info->module->internal.coroutinesNamespace = std::dynamic_pointer_cast<DET::Namespace>(info->module->internal.module->exports->findAll("Coroutines")[0]);
    info->module->internal.schedulerClass = std::dynamic_pointer_cast<DET::Class>(info->module->internal.coroutinesNamespace->scope->findAll("Scheduler")[0]);
    info->module->internal.coroutinesModule = Util::getModule(info->module->internal.schedulerClass->parentScope.lock().get()).lock();
    info->module->internal.resultClass = std::dynamic_pointer_cast<DET::Class>(info->module->internal.module->exports->findAll("Result")[0]);

    info->module->internal.schedulerVariable = std::make_shared<DET::Variable>(
      "$scheduler",
      std::make_shared<DET::Type>(
        info->module->internal.schedulerClass,
        DET::Type::createModifierVector({ { TypeModifierFlag::Reference } })
      ),
      AltaCore::Errors::Position(0, 0, info->module->internal.module->path),
      info->module->scope
    );
    info->module->scope->items.push_back(info->module->internal.schedulerVariable);

    auto schedulerClassAlias = std::make_shared<DET::Alias>(
      "Scheduler",
      info->module->internal.schedulerClass,
      AltaCore::Errors::Position(0, 0, info->module->internal.module->path),
      info->module->scope
    );
    info->module->scope->items.push_back(schedulerClassAlias);

    auto resultClassAlias = std::make_shared<DET::Alias>(
      "Result",
      info->module->internal.resultClass,
      AltaCore::Errors::Position(0, 0, info->module->internal.module->path),
      info->module->scope
    );
    info->module->scope->items.push_back(resultClassAlias);

    auto nativeNamespace = std::dynamic_pointer_cast<DET::Namespace>(info->module->internal.module->exports->findAll("Native")[0]);
    info->module->internal.metaCoroutineClass = std::dynamic_pointer_cast<DET::Class>(nativeNamespace->scope->findAll("_Alta_basic_coroutine")[0]);
  };

  if (info->module->packageInfo.root == Modules::standardLibraryPath / "_internal" && info->module->name == "_internal/main") {
    info->module->internal.module = info->module;
  } else {
    auto internalImport = std::make_shared<ImportStatement>("@internal@", "@internal@");
    auto internalDetail = internalImport->fullDetail(info->module->scope);
    info->dependencyASTs.push_back(internalDetail->importedAST);

    info->module->internal.module = internalDetail->importedModule;

    std::function<bool(std::shared_ptr<DET::Module>)> hasInternalParent = [&](std::shared_ptr<DET::Module> mod) -> bool {
      if (mod->packageInfo.root == Modules::standardLibraryPath / "_internal") {
        return true;
      }
      if (mod->parentModule && hasInternalParent(mod->parentModule)) {
        return true;
      }
      return false;
    };

    if (!info->module->parentModule || !hasInternalParent(info->module->parentModule)) {
      detailInternal();
    }
  }

  for (auto& stmt: statements) {
    auto det = stmt->fullDetail(info->module->scope);
    info->statements.push_back(det);
    if (stmt->nodeType() == NodeType::ImportStatement) {
      auto import = std::dynamic_pointer_cast<ImportStatement>(stmt);
      auto importDet = std::dynamic_pointer_cast<DH::ImportStatement>(det);
      info->dependencyASTs.push_back(importDet->importedAST);
    } else if (stmt->nodeType() == NodeType::ExportStatement) {
      auto statement = std::dynamic_pointer_cast<ExportStatement>(stmt);
      auto statementDet = std::dynamic_pointer_cast<DH::ExportStatement>(det);
      if (statement->externalTarget) {
        info->dependencyASTs.push_back(statementDet->externalTarget->importedAST);
      }
    }
  }

  // we need to detail the internal information *after* detailing the module for the `_internal` package
  if (info->module->packageInfo.root == Modules::standardLibraryPath / "_internal" && info->module->name == "_internal/main") {
    detailInternal();

    std::function<void(std::shared_ptr<DET::Module>)> addDetails = [&](std::shared_ptr<DET::Module> mod) {
      mod->internal = info->module->internal;
      for (auto& dependency: mod->dependencies) {
        if (!(dependency->packageInfo.root == Modules::standardLibraryPath / "_internal" && dependency->name == "_internal/main")) {
          addDetails(dependency);
        }
      }
    };
    for (auto& dependency: info->module->dependencies) {
      addDetails(dependency);
    }
  }
};
void AltaCore::AST::RootNode::detail(std::string filePath, std::string moduleName) {
  return detail(Filesystem::Path(filePath), moduleName);
};

ALTACORE_AST_VALIDATE_D(RootNode) {
  ALTACORE_VS_SS;
  for (size_t i = 0; i< statements.size(); i++) {
    auto& stmt = statements[i];
    auto& stmtDet = info->statements[i];
    if (!stmt) ALTACORE_VALIDATION_ERROR("empty statement in root node");
    stmt->validate(stack, stmtDet);
  }
  ALTACORE_VS_E;
};
