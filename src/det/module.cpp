#include "../../include/altacore/det/module.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Module::nodeType() {
  return NodeType::Module;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Module::clone() {
  return std::make_shared<Module>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Module::deepClone() {
  auto self = std::dynamic_pointer_cast<Module>(clone());
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());
  self->scope->parentModule = self;
  return self;
};

std::shared_ptr<AltaCore::DET::Module> AltaCore::DET::Module::create(std::string name, AltaCore::Filesystem::Path path) {
  auto mod = std::make_shared<Module>();
  mod->name = name;
  mod->path = path;
  mod->scope = std::make_shared<Scope>(mod);

  return mod;
};

AltaCore::DET::Module::Module() {};
