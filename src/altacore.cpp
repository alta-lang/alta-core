#include "../include/altacore.hpp"

void AltaCore::registerGlobalAttributes() {
  Attributes::registerAttribute({ "read" }, { AST::NodeType::FunctionDefinitionNode }, [](std::shared_ptr<AST::Node> _target, std::vector<Attributes::AttributeArgument> args) -> void {
    auto target = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(_target);
    if (!target) throw std::runtime_error("this isn't supposed to happen");

    target->$function->isAccessor = true;
  });
};
