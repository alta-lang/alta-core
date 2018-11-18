#ifndef ALTACORE_MODULES_HPP
#define ALTACORE_MODULES_HPP

#include "fs.hpp"
#include "det-shared.hpp"
#include "ast/root-node.hpp"
#include <string>
#include <exception>
#include <functional>

namespace AltaCore {
  namespace Modules {
    class ModuleError: public std::exception {};
    class ModuleResolutionError: public ModuleError {};
    class InvalidPackageInformationError: public ModuleError {};
    class PackageInformationNotFoundError: public ModuleError {};

    struct PackageVersion {
      unsigned long major = 0;
      unsigned long minor = 0;
      unsigned long patch = 0;
      std::string toString();
    };
    struct PackageInfo {
      std::string name;
      PackageVersion version;
      Filesystem::Path root;
      Filesystem::Path main;
    };

    extern Filesystem::Path stlPath;
    extern std::function<std::shared_ptr<AST::RootNode>(std::string importRequest, Filesystem::Path requestingModulePath)> parseModule;
    Filesystem::Path resolve(std::string importRequest, Filesystem::Path relativeTo);
    Filesystem::Path findInfo(Filesystem::Path moduleOrPackagePath);
    PackageInfo getInfo(Filesystem::Path moduleOrPackagePath, bool findInfo = true);
  };
};

#endif // ALTACORE_MODULES_HPP
