#include "../../include/altacore/ast/root-node.hpp"
#include "../../include/altacore/modules.hpp"
#include "../../include/altacore/ast/import-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RootNode::nodeType() {
  return NodeType::RootNode;
};

AltaCore::AST::RootNode::RootNode() {};
AltaCore::AST::RootNode::RootNode(std::vector<std::shared_ptr<AltaCore::AST::StatementNode>> _statements):
  statements(_statements)
  {};

void AltaCore::AST::RootNode::detail(AltaCore::Filesystem::Path filePath, std::string moduleName) {
  if (moduleName == "") {
    // attempt to find the module name
    try {
      auto info = Modules::getInfo(filePath);
      auto relativeFilePath = filePath.relativeTo(info.root);
      moduleName = info.name + '/' + (relativeFilePath.dirname() / relativeFilePath.filename()).toString("/");
    } catch (const Modules::ModuleError&) {
      moduleName = (filePath.dirname() / filePath.filename()).uproot().toString("/");
    }
  }
  $module = DET::Module::create(moduleName, filePath);
  $module->ast = shared_from_this();

  for (auto& stmt: statements) {
    stmt->detail($module->scope);
    if (stmt->nodeType() == NodeType::ImportStatement) {
      auto import = std::dynamic_pointer_cast<ImportStatement>(stmt);
      $dependencyASTs.push_back(import->$importedAST);
    }
  }
};
void AltaCore::AST::RootNode::detail(std::string filePath, std::string moduleName) {
  return detail(Filesystem::Path(filePath, moduleName));
};
