#ifndef ALTACORE_MODULES_HPP
#define ALTACORE_MODULES_HPP

#include "fs.hpp"
#include "det-shared.hpp"
#include <string>
#include <exception>
#include <functional>

#ifdef ALTACORE_LOCAL_SEMVER
#include "../../deps/semver.c/semver.h"
#else
#include "semver.h"
#endif

namespace AltaCore {
  namespace AST {
    class RootNode; // forward declaration
  };
  namespace Modules {
    class ModuleError: public std::exception {};
    class ModuleResolutionError: public ModuleError {};
    class InvalidPackageInformationError: public ModuleError {};
    class PackageInformationNotFoundError: public ModuleError {};

    enum class OutputBinaryType {
      Library,
      Exectuable,
    };

    struct TargetInfo {
      std::string name;
      Filesystem::Path main;
      OutputBinaryType type = OutputBinaryType::Library;
    };

    struct PackageInfo {
      std::string name;
      semver_t version;
      Filesystem::Path root;
      Filesystem::Path main;
      OutputBinaryType outputBinary = OutputBinaryType::Library;
      std::vector<TargetInfo> targets;

      bool isEntryPackage = false;

      PackageInfo();
    };

    extern Filesystem::Path standardLibraryPath;
    extern std::function<std::shared_ptr<AST::RootNode>(std::string importRequest, Filesystem::Path requestingModulePath)> parseModule;
    Filesystem::Path resolve(std::string importRequest, Filesystem::Path relativeTo);
    Filesystem::Path findInfo(Filesystem::Path moduleOrPackagePath);
    PackageInfo getInfo(Filesystem::Path moduleOrPackagePath, bool findInfo = true);
  };
};

#endif // ALTACORE_MODULES_HPP
