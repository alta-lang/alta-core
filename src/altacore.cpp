#include "../include/altacore.hpp"

void AltaCore::registerGlobalAttributes() {
  Attributes::registerAttribute({ "read" }, { AST::NodeType::FunctionDefinitionNode }, [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void {
    auto target = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(_target);
    auto info = std::dynamic_pointer_cast<DH::FunctionDefinitionNode>(_info);
    if (!target || !info) throw std::runtime_error("this isn't supposed to happen 1");

    info->function->isAccessor = true;
  });
  Attributes::registerAttribute({ "copy" }, { AST::NodeType::ClassSpecialMethodDefinitionStatement }, [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void {
    auto target = std::dynamic_pointer_cast<AST::ClassSpecialMethodDefinitionStatement>(_target);
    auto info = std::dynamic_pointer_cast<DH::ClassSpecialMethodDefinitionStatement>(_info);
    if (!target || !info) throw std::runtime_error("this isn't supposed to happen 2");

    info->isCopyConstructor = true;
  });
  Attributes::registerAttribute({ "external" }, { AST::NodeType::StructureDefinitionStatement }, [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void {
    auto target = std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(_target);
    auto info = std::dynamic_pointer_cast<DH::StructureDefinitionStatement>(_info);
    if (!target || !info) throw std::runtime_error("this isn't suppossed to happen 3");

    target->isExternal = info->isExternal = info->structure->isExternal = true;
  });
  Attributes::registerAttribute({ "typed" }, { AST::NodeType::StructureDefinitionStatement }, [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void {
    auto target = std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(_target);
    auto info = std::dynamic_pointer_cast<DH::StructureDefinitionStatement>(_info);
    if (!target || !info) throw std::runtime_error("this isn't suppossed to happen 3");

    target->isTyped = info->isTyped = info->structure->isTyped = true;
  });
};
