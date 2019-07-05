#include "../include/altacore.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace AltaCore {
  namespace Modules {
    std::vector<Filesystem::Path> prioritySearchPaths;
    std::vector<Filesystem::Path> searchPaths;
    Filesystem::Path standardLibraryPath;
    ALTACORE_MAP<std::string, std::shared_ptr<AltaCore::AST::RootNode>> importCache;
    ALTACORE_MAP<std::string, Parser::PrepoExpression> defaultDefinitions;
    ALTACORE_MAP<std::string, Parser::PrepoExpression>* parsingDefinitions = &defaultDefinitions;
    std::function<std::shared_ptr<AST::RootNode>(std::string importRequest, Filesystem::Path requestingModulePath)> parseModule = [](std::string importRequest, Filesystem::Path requestingModulePath) -> std::shared_ptr<AST::RootNode> {
      auto modPath = resolve(importRequest, requestingModulePath);
      if (importCache.find(modPath.absolutify().toString()) != importCache.end()) {
        return importCache[modPath.absolutify().toString()];
      }
      std::ifstream file(modPath.absolutify().toString());
      std::string line;
      Lexer::Lexer lexer(modPath);

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

      Parser::Parser parser(lexer.tokens, *parsingDefinitions, modPath);
      parser.parse();
      auto root = std::dynamic_pointer_cast<AST::RootNode>(*parser.root);
      //root->detail(modPath);

      importCache[modPath.absolutify().toString()] = root;

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
  if (yamlRoot["type"]) {
    auto out = yamlRoot["type"].as<std::string>();
    if (Util::stringsAreEqualCaseInsensitive(out, "executable") || Util::stringsAreEqualCaseInsensitive(out, "exe")) {
      info.outputBinary = OutputBinaryType::Exectuable;
    } else if (Util::stringsAreEqualCaseInsensitive(out, "library") || Util::stringsAreEqualCaseInsensitive(out, "lib")) {
      info.outputBinary = OutputBinaryType::Library;
    } else {
      throw InvalidPackageInformationError();
    }
  }
  if (yamlRoot["targets"]) {
    auto targets = yamlRoot["targets"];
    if (!targets.IsSequence()) {
      throw InvalidPackageInformationError();
    }
    for (size_t i = 0; i < targets.size(); i++) {
      auto target = targets[i];
      if (!target["main"]) {
        throw InvalidPackageInformationError();
      }
      TargetInfo targetInfo;
      targetInfo.main = Filesystem::Path(target["main"].as<std::string>()).absolutify(info.root);
      if (target["name"]) {
        targetInfo.name = target["name"].as<std::string>();
      } else {
        targetInfo.name = targetInfo.main.filename();
      }
      if (target["type"]) {
        auto type = target["type"].as<std::string>();
        if (Util::stringsAreEqualCaseInsensitive(type, "executable") || Util::stringsAreEqualCaseInsensitive(type, "exe")) {
          targetInfo.type = OutputBinaryType::Exectuable;
        } else if (Util::stringsAreEqualCaseInsensitive(type, "library") || Util::stringsAreEqualCaseInsensitive(type, "lib")) {
          targetInfo.type = OutputBinaryType::Library;
        } else {
          throw InvalidPackageInformationError();
        }
      }
      info.targets.push_back(targetInfo);
    }
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
    // try the priority search paths
    for (auto& path: prioritySearchPaths) {
      auto maybePath = path / importPath;
      if (maybePath.exists()) {
        if (maybePath.isDirectory()) {
          auto info = getInfo(maybePath);
          if (info.main.isValid()) {
            maybePath = info.main;
          } else {
            continue;
          }
        }
        return maybePath.normalize();
      } else if ((maybePath = maybePath + ".alta").exists()) {
        return maybePath.normalize();
      }
    }
    // try resolving it in a package
    while (!relativeTo.isRoot()) {
      auto modFilePath = (relativeTo / "alta-packages" / importPath).normalize();
      if (modFilePath.exists()) {
        return modFilePath;
      }
      relativeTo.pop();
    }
    // try the regular search paths
    for (auto& path: searchPaths) {
      auto maybePath = path / importPath;
      if (maybePath.exists()) {
        if (maybePath.isDirectory()) {
          auto info = getInfo(maybePath);
          if (info.main.isValid()) {
            maybePath = info.main;
          } else {
            continue;
          }
        }
        return maybePath.normalize();
      } else if ((maybePath = maybePath + ".alta").exists()) {
        return maybePath.normalize();
      }
    }
  } else if (importPath.components.size() > 1 && (importPath.components[0] == "." || importPath.components[0] == "..")) {
    // resolve locally
    if (!relativeTo.isDirectory()) {
      relativeTo = relativeTo.dirname();
    }
    auto truePath = importPath.absolutify(relativeTo);
    if (truePath.exists()) {
      auto info = getInfo(truePath);
      if (info.main.isValid()) {
        return info.main;
      }
    }
  } else {
    // try the priority search paths (they're the most important)
    for (auto& path: prioritySearchPaths) {
      auto maybePath = path / importPath;
      if (maybePath.exists()) {
        if (maybePath.isDirectory()) {
          auto info = getInfo(maybePath);
          if (info.main.isValid()) {
            maybePath = info.main;
          } else {
            continue;
          }
        }
        return maybePath.normalize();
      } else if ((maybePath = maybePath + ".alta").exists()) {
        return maybePath.normalize();
      }
    }
    // try stdlib next (it has the next highest precedence)
    auto stdlibResolved = importPath.absolutify(standardLibraryPath);
    if (stdlibResolved.exists()) {
      auto info = getInfo(stdlibResolved); // stdlibResolved should be a package path, not a module path
      if (info.main) {
        return info.main;
      }
      // welp, this shouldn't ever happen. but just in case...
      return stdlibResolved / "main.alta";
    } else if ((stdlibResolved = stdlibResolved + ".alta").exists()) {
      return stdlibResolved;
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
      // finally, try the regular search paths
      for (auto& path: searchPaths) {
        auto maybePath = path / importPath;
        if (maybePath.exists()) {
          if (maybePath.isDirectory()) {
            auto info = getInfo(maybePath);
            if (info.main.isValid()) {
              maybePath = info.main;
            } else {
              continue;
            }
          }
          return maybePath.normalize();
        } else if ((maybePath = maybePath + ".alta").exists()) {
          return maybePath.normalize();
        }
      }
    }
  }

  throw ModuleResolutionError();
};
