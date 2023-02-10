#ifndef ALTACORE_MODULES_HPP
#define ALTACORE_MODULES_HPP

#include "fs.hpp"
#include "det-shared.hpp"
#include <string>
#include <exception>
#include <functional>
#include <stdexcept>
#include <memory>
#include "simple-map.hpp"

#ifdef ALTACORE_LOCAL_SEMVER
#include "../../deps/semver.c/semver.h"
#else
#include "semver.h"
#endif

namespace AltaCore {
  namespace AST {
    class RootNode; // forward declaration
  };
  namespace Parser {
    class PrepoExpression;
  };
  namespace Modules {
    class ModuleError: public std::runtime_error {
      public:
        ModuleError():
          runtime_error("ModuleError")
          {};
        ModuleError(std::string what):
          runtime_error("ModuleError: " + what)
          {};
      protected:
        ModuleError(std::string what, bool override):
          runtime_error(what)
          {};
    };
    class ModuleResolutionError: public ModuleError {
      public:
        ModuleResolutionError(AltaCore::Filesystem::Path from, std::string to):
          ModuleError("ModuleResolutionError: failed to resolve \"" + to + "\" from " + from.toString(), true)
          {};
    };
    class InvalidPackageInformationError: public ModuleError {
      public:
        InvalidPackageInformationError():
          ModuleError("InvalidPackageInformationError", true)
          {};
    };
    class PackageInformationNotFoundError: public ModuleError {
      public:
        PackageInformationNotFoundError():
          ModuleError("PackageInformationNotFoundError", true)
          {};
    };

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

    extern std::vector<Filesystem::Path> prioritySearchPaths;
    extern std::vector<Filesystem::Path> searchPaths;
    extern Filesystem::Path standardLibraryPath;
    extern ALTACORE_MAP<std::string, Parser::PrepoExpression>* parsingDefinitions;
    extern std::function<std::shared_ptr<AST::RootNode>(std::string importRequest, Filesystem::Path requestingModulePath)> parseModule;
    Filesystem::Path resolve(std::string importRequest, Filesystem::Path relativeTo);
    Filesystem::Path findInfo(Filesystem::Path moduleOrPackagePath);
    PackageInfo getInfo(Filesystem::Path moduleOrPackagePath, bool findInfo = true);
  };
};

#endif // ALTACORE_MODULES_HPP
