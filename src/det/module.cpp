#include "../include/altacore/det/module.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Module::nodeType() {
  return NodeType::Module;
};

AltaCore::DET::Module* AltaCore::DET::Module::clone() {
  return new Module(*this);
};

AltaCore::DET::Module* AltaCore::DET::Module::deepClone() {
  Module* self = clone();
  self->scope = scope->deepClone();
  return self;
};

AltaCore::DET::Module::Module(std::string _name, AltaCore::Filesystem::Path _path):
  name(_name),
  path(_path),
  scope(new Scope(this))
  {};