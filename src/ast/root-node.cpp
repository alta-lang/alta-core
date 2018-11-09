#include "../include/altacore/ast/root-node.hpp"
#include "../include/altacore/modules.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RootNode::nodeType() {
  return NodeType::RootNode;
};

AltaCore::AST::RootNode::RootNode() {};
AltaCore::AST::RootNode::RootNode(std::vector<AltaCore::AST::StatementNode*> _statements):
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
  $module = new DET::Module(moduleName);

  for (auto& stmt: statements) {
    stmt->detail($module->scope);
  }
};
void AltaCore::AST::RootNode::detail(std::string filePath, std::string moduleName) {
  return detail(Filesystem::Path(filePath, moduleName));
};