#include "../include/altacore.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace AltaCore {
  namespace Modules {
    Filesystem::Path standardLibraryPath;
    std::function<std::shared_ptr<AST::RootNode>(std::string importRequest, Filesystem::Path requestingModulePath)> parseModule = [](std::string importRequest, Filesystem::Path requestingModulePath) -> std::shared_ptr<AST::RootNode> {
      auto modPath = resolve(importRequest, requestingModulePath);
      std::ifstream file(modPath.toString());
      std::string line;
      Lexer::Lexer lexer;

      if (!file.is_open()) {
        throw std::runtime_error("oh no.");
      }

      while (std::getline(file, line)) {
        if (file.peek() != EOF) {
          line += "\n";
        }
        lexer.feed(line);
      }

      file.close();

      Parser::Parser parser(lexer.tokens);
      parser.parse();
      auto root = std::dynamic_pointer_cast<AST::RootNode>(parser.root.value());
      root->detail(modPath);

      return root;
    };
  };
};

AltaCore::Modules::PackageInfo::PackageInfo() {
  if (semver_parse("0.0.0", &version) != 0) {
    throw std::runtime_error("this is semver parsing error that should never happen");
  }
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
  auto infoPath = moduleOrPackagePath;
  if (_findInfo) {
    infoPath = findInfo(moduleOrPackagePath);
    if (!infoPath) {
      throw PackageInformationNotFoundError();
    }
  }
  auto yamlRoot = YAML::LoadFile(infoPath.toString());
  if (!yamlRoot["name"]) {
    throw InvalidPackageInformationError();
  }
  PackageInfo info;
  info.root = infoPath.dirname();
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
    auto str = yamlRoot["version"].as<std::string>();
    semver_parse(str.c_str(), &info.version);
  }
  return info;
};

AltaCore::Filesystem::Path AltaCore::Modules::resolve(std::string importRequest, AltaCore::Filesystem::Path relativeTo) {
  using namespace Filesystem;

  relativeTo = relativeTo.normalize();
  auto importPath = Path(importRequest, std::string(1, '/'));
  if (importPath.extname() == "alta") {
    // resolve locally
    if (!relativeTo.isDirectory()) {
      relativeTo = relativeTo.dirname();
    }
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
    // try stdlib first (it takes precedence)
    auto stdlibResolved = importPath.absolutify(standardLibraryPath);
    if (stdlibResolved.exists()) {
      auto info = getInfo(stdlibResolved); // stdlibResolved should be a package path, not a module path
      if (info.main) {
        return info.main;
      }
      // welp, this shouldn't ever happen. but just in case...
      return stdlibResolved / "main.alta";
    } else {
      // otherwise, try finding the module in a package
      while (!relativeTo.isRoot()) {
        auto modFolderPath = relativeTo / "alta-packages" / importPath;
        if (modFolderPath.exists()) {
          auto info = getInfo(modFolderPath);
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
