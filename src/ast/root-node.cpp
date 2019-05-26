#include "../../include/altacore/ast/root-node.hpp"
#include "../../include/altacore/modules.hpp"
#include "../../include/altacore/ast/import-statement.hpp"
#include "../../include/altacore/ast/export-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RootNode::nodeType() {
  return NodeType::RootNode;
};

AltaCore::AST::RootNode::RootNode() {};
AltaCore::AST::RootNode::RootNode(std::vector<std::shared_ptr<AltaCore::AST::StatementNode>> _statements):
  statements(_statements)
  {};

void AltaCore::AST::RootNode::detail(AltaCore::Filesystem::Path filePath, std::string moduleName) {
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
      moduleName = (filePath.dirname() / filePath.filename()).uproot().toString("/");
      pkgInfo.main = filePath;
      pkgInfo.root = filePath.dirname();
    }
  } else {
    try {
      pkgInfo = Modules::getInfo(filePath);
    } catch (...) {
      pkgInfo.main = filePath;
      pkgInfo.name = moduleName;
      pkgInfo.root = filePath.dirname();
    }
  }
  info->module = DET::Module::create(moduleName, pkgInfo, filePath);
  info->module->ast = shared_from_this();

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
