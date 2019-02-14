#include "../include/altacore.hpp"

void AltaCore::registerGlobalAttributes() {
  Attributes::registerAttribute({ "read" }, { AST::NodeType::FunctionDefinitionNode }, [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void {
    auto target = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(_target);
    auto info = std::dynamic_pointer_cast<DH::FunctionDefinitionNode>(_info);
    if (!target || !info) throw std::runtime_error("this isn't supposed to happen");

    info->function->isAccessor = true;
  });
};
