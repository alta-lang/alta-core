#include "../include/altacore/modules.hpp"
#include <yaml-cpp/yaml.h>

namespace AltaCore {
  namespace Modules {
    Filesystem::Path stlPath;
  };
};

std::string AltaCore::Modules::PackageVersion::toString() {
  return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch);
};

AltaCore::Filesystem::Path AltaCore::Modules::findInfo(AltaCore::Filesystem::Path moduleOrPackagePath) {
  while (!moduleOrPackagePath.isRoot()) {
    auto infoPath = moduleOrPackagePath / "package.alta.yaml";
    if (infoPath.exists()) {
      return infoPath;
    }
    moduleOrPackagePath.pop();
  }
  return Filesystem::Path();
};

AltaCore::Modules::PackageInfo AltaCore::Modules::getInfo(AltaCore::Filesystem::Path moduleOrPackagePath, bool _findInfo) {
  if (_findInfo) {
    moduleOrPackagePath = findInfo(moduleOrPackagePath);
  }
  if (!moduleOrPackagePath.isValid()) {
    throw PackageInformationNotFoundError();
  }
  auto yamlRoot = YAML::LoadFile(moduleOrPackagePath.toString());
  if (!yamlRoot["name"]) {
    throw InvalidPackageInformationError();
  }
  PackageInfo info;
  info.root = moduleOrPackagePath.dirname();
  info.name = yamlRoot["name"].as<std::string>();
  if (yamlRoot["main"]) {
    info.main = Filesystem::Path(yamlRoot["main"].as<std::string>()).absolutify(info.root);
  } else {
    info.main = info.root / "main.alta";
    if (!info.main.exists()) {
      info.main = info.main.dirname() / "src" / "main.alta";
      if (!info.main.exists()) {
        info.main = Filesystem::Path();
      }
    }
  }
  if (yamlRoot["version"]) {
    auto verString = yamlRoot["version"].as<std::string>();
    auto firstDot = verString.find_first_of('.');
    auto secondDot = verString.find_last_of('.');
    info.version.major = std::stoul(verString.substr(0, firstDot));
    info.version.minor = std::stoul(verString.substr(firstDot + 1, secondDot));
    info.version.patch = std::stoul(verString.substr(secondDot + 1));
  }
  return info;
};

AltaCore::Filesystem::Path AltaCore::Modules::resolve(std::string importRequest, AltaCore::Filesystem::Path relativeTo) {
  using namespace Filesystem;

  relativeTo = relativeTo.normalize();
  auto importPath = Path(importRequest, std::string(1, '/'));
  if (importPath.extname() == ".alta") {
    // resolve locally
    auto truePath = importPath.absolutify(relativeTo);
    if (truePath.exists()) {
      return truePath;
    }
    // try resolving it in a package
    while (!relativeTo.isRoot()) {
      auto modFilePath = (relativeTo / "alta-packages" / importPath).normalize();
      if (modFilePath.exists()) {
        return modFilePath;
      }
      relativeTo.pop();
    }
  } else {
    // try stl first (it takes precedence)
    auto stlResolved = importPath.absolutify(stlPath) + ".alta";
    if (stlResolved.exists()) {
      return stlResolved;
    } else {
      // otherwise, try finding the module in a package
      while (!relativeTo.isRoot()) {
        auto modFolderPath = relativeTo / "alta-packages" / importPath;
        if (modFolderPath.exists()) {
          PackageInfo info = getInfo(modFolderPath);
          if (info.main.isValid()) {
            return info.main;
          }
        }
        auto modFilePath = modFolderPath + ".alta";
        if (modFilePath.exists()) {
          return modFilePath;
        }
        relativeTo.pop();
      }
    }
  }

  throw ModuleResolutionError();
};