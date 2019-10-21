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
  mod->exports = std::make_shared<Scope>(mod);
  if (path) {
    try {
      mod->packageInfo = Modules::getInfo(path);
    } catch (...) {
      // do nothing
    }
  }

  return mod;
};
std::shared_ptr<AltaCore::DET::Module> AltaCore::DET::Module::create(std::string name, AltaCore::Modules::PackageInfo packageInfo, AltaCore::Filesystem::Path path) {
  auto mod = std::make_shared<Module>();
  mod->name = name;
  mod->path = path;
  mod->scope = std::make_shared<Scope>(mod);
  mod->exports = std::make_shared<Scope>(mod);
  mod->packageInfo = packageInfo;

  return mod;
};

AltaCore::DET::Module::Module() {};

std::string AltaCore::DET::Module::toString() const {
  auto versionString = std::to_string(packageInfo.version.major) + '.' + std::to_string(packageInfo.version.minor) + '.' + std::to_string(packageInfo.version.patch);
  if (packageInfo.version.prerelease != NULL) {
    versionString += '-' + std::string(packageInfo.version.prerelease);
  }
  if (packageInfo.version.metadata != NULL) {
    versionString += '+' + std::string(packageInfo.version.metadata);
  }
  return name + '@' + versionString;
};
