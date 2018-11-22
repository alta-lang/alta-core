#include "../include/altacore/fs.hpp"
#include <memory>
#include <functional>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#endif

bool AltaCore::Filesystem::exists(AltaCore::Filesystem::Path path) {
#if defined(_WIN32) || defined(_WIN64)
  const auto statFunc = _stat;
  typedef struct _stat statStruct;
#else
  const auto statFunc = stat;
  typedef struct stat statStruct;
#endif

  auto str = path.absolutify().toString();
  auto cstrPath = str.c_str();
  statStruct buf;
  return (statFunc(cstrPath, &buf) == 0);
};

bool AltaCore::Filesystem::mkdirp(AltaCore::Filesystem::Path path) {
  // we could be more efficient by traversing the components
  // backwards and until we reach the first existent path (or a root)
  // and then create recursively starting there
  // but ¯\_(ツ)_/¯
  auto refPath = path.absolutify();
  path.components.clear();
  for (auto& component: refPath.components) {
    path.push(component);
    if (!path.exists()) {
#if defined(_WIN32) || defined(_WIN64)
      const auto mkdirFunc = _mkdir;
#else
      auto mkdirFunc = std::bind(mkdir, std::placeholders::_1, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
      auto str = path.toString();
      auto ok = mkdirFunc(str.c_str()) == 0;
      // we already checked for existence, but
      // just in case, let's make sure that wasn't the error
      if (!ok && errno != EEXIST) return false;
    }
  }
  return true;
};

std::string AltaCore::Filesystem::cwd() {
#if defined(_WIN32) || defined(_WIN64)
  auto requiredBufferLength = GetCurrentDirectory(0, NULL);
  auto buf = std::make_unique<char[]>(requiredBufferLength);
  GetCurrentDirectory(requiredBufferLength, buf.get());
  return std::string(buf.get());
#else
  char tmp[MAXPATHLEN];
  return (getcwd(tmp, MAXPATHLEN) ? std::string(tmp) : std::string());
#endif
};

AltaCore::Filesystem::Path::Path() {};
AltaCore::Filesystem::Path::Path(std::string path, std::vector<std::string> separators) {
  std::vector<size_t> separatorLengths;
  size_t startOfLatestComponent = 0;
  size_t endOfLatestComponent = 0;
  bool hasContent = false;

  for (auto& separator: separators) {
    separatorLengths.push_back(separator.length());
  }

  for (size_t i = 0; i < path.length(); i++) {
    bool found = false;
    for (size_t j = 0; j < separators.size(); j++) {
      auto& separator = separators[j];
      auto& separatorLength = separatorLengths[j];
      bool ok = false;
      if (path[i] == separator[0]) {
        for (size_t j = i, k = 1; j < path.length(); j++, k++) {
          if (k >= separatorLength) {
            if (hasContent) {
              components.push_back(path.substr(startOfLatestComponent, endOfLatestComponent - startOfLatestComponent + 1));
            } else {
              components.push_back(std::string());
            }
            i = j;
            startOfLatestComponent = i + 1;
            endOfLatestComponent = i + 1;
            ok = true;
            break;
          } else if (path[j] != separator[k]) {
            break;
          }
        }
      }
      if (ok) {
        found = true;
        break;
      }
    }
    if (!found) {
      hasContent = true;
      endOfLatestComponent = i;
    }
  }

  if (hasContent) {
    components.push_back(path.substr(startOfLatestComponent, endOfLatestComponent - startOfLatestComponent + 1));
  }

  if (components.size() > 0) {
    if (components[0] == "") {
      hasRoot = true;
      root = "";
      components.erase(components.begin());
    } else if (components[0][components[0].length() - 1] == ':') {
      hasRoot = true;
      root = components[0];
      components.erase(components.begin());
    }
  }
};
AltaCore::Filesystem::Path::Path(std::string _path, std::string _separator):
  AltaCore::Filesystem::Path::Path(_path, std::vector<std::string> { _separator })
  {};

void AltaCore::Filesystem::Path::absolutifyInPlace(AltaCore::Filesystem::Path relativeTo) {
  // btw, we're free to modify `relativeTo` directly, since it's passed in by value
  if (hasRoot) return;
  for (auto& component: components) {
    if (component == ".") {
      // ignore
    } else if (component == "..") {
      if (relativeTo.components.size() > 0) {
        relativeTo.components.pop_back();
      }
    } else {
      relativeTo.components.push_back(component);
    }
  }
  hasRoot = relativeTo.hasRoot;
  root = relativeTo.root;
  components = relativeTo.components;
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::normalize() {
  auto newPath = Path(*this);
  newPath.components.clear();
  for (auto& component: components) {
    if (component == ".") {
      // ignore
    } else if (component == "..") {
      if (newPath.components.size() > 0) {
        newPath.components.pop_back();
      }
    } else {
      newPath.components.push_back(component);
    }
  }
  return newPath;
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::absolutify(AltaCore::Filesystem::Path relativeTo) {
  // btw, we're free to modify `relativeTo` directly, since it's passed in by value
  auto newPath = Path(*this);
  newPath.absolutifyInPlace(relativeTo);
  return newPath;
};
AltaCore::Filesystem::Path AltaCore::Filesystem::Path::absolutify(std::string relativeTo) {
  return absolutify(Path(relativeTo));
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::relativeTo(AltaCore::Filesystem::Path other) {
  auto newPath = absolutify();
  other = other.absolutify();
  while (newPath.components.size() > 0 && other.components.size() > 0 && newPath.components[0] == other.components[0]) {
    newPath.shift();
    other.shift();
  }
  if (other.components.size() > 0) {
    other.shift();
  }
  for (auto& component: other.components) {
    newPath.unshift("..");
  }
  newPath.hasRoot = false;
  newPath.root = "";
  return newPath;
};

std::string AltaCore::Filesystem::Path::toString(std::string separator) {
  std::string result;

  if (hasRoot) {
    result += root;
    result += separator;
  }

  bool isFirst = true;
  for (auto& component: components) {
    if (isFirst) {
      isFirst = false;
    } else {
      result += separator;
    }
    result += component;
  }

  return result;
};
std::string AltaCore::Filesystem::Path::toString(char separator) {
  return toString(std::string(1, separator));
};

std::string AltaCore::Filesystem::Path::basename() {
  if (!hasComponents()) return "";
  auto size = components.size();
  if (size < 1) return "";
  return components[size - 1];
};

std::string AltaCore::Filesystem::Path::filename() {
  if (!hasComponents()) return "";
  auto base = basename();
  auto pos = base.find_last_of('.');
  if (pos == std::string::npos) return base;
  return base.substr(0, pos);
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::dirname() {
  if (!hasComponents()) return Path();
  auto newPath = Path(*this);
  newPath.components.pop_back();
  return newPath;
};

std::string AltaCore::Filesystem::Path::extname() {
  if (!hasComponents()) return "";
  auto base = basename();
  return base.substr(base.find_last_of('.') + 1); // doesn't include the '.'
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::uproot() {
  auto newPath = Path(*this);
  newPath.hasRoot = false;
  newPath.root = "";
  return newPath;
};

void AltaCore::Filesystem::Path::push(std::string component) {
  return components.push_back(component);
};
std::string AltaCore::Filesystem::Path::pop() {
  if (!hasComponents()) return "";
  auto component = components[components.size() - 1];
  components.pop_back();
  return component;
};
void AltaCore::Filesystem::Path::unshift(std::string component) {
  components.insert(components.begin(), component);
};
std::string AltaCore::Filesystem::Path::shift() {
  if (!hasComponents()) return "";
  auto component = components[0];
  components.erase(components.begin());
  return component;
};

bool AltaCore::Filesystem::Path::isValid() {
  return (hasRoot || components.size() > 0);
};
bool AltaCore::Filesystem::Path::isEmpty() {
  return !isValid();
};
bool AltaCore::Filesystem::Path::isRoot() {
  return (hasRoot && components.size() == 0);
};
bool AltaCore::Filesystem::Path::hasComponents() {
  return components.size() > 0;
};
bool AltaCore::Filesystem::Path::exists() {
  // sure, Windows has regular ol' `stat`, too,
  // but Windows c++ api documentation recommends
  // `_stat` instead. it's fully compatible with
  // stat for our purposes here so ¯\_(ツ)_/¯
#if defined(_WIN32) || defined(_WIN64)
  const auto statFunc = _stat;
  typedef struct _stat statStruct;
#else
  const auto statFunc = stat;
  typedef struct stat statStruct;
#endif

  auto path = absolutify().toString();
  auto cstrPath = path.c_str();
  statStruct buf;
  return (statFunc(cstrPath, &buf) == 0);
};
bool AltaCore::Filesystem::Path::isDirectory() {
  // `Path().exists` for the rationale for using `_stat` instead of `stat` on Windows
#if defined(_WIN32) || defined(_WIN64)
  const auto statFunc = _stat;
  typedef struct _stat statStruct;
#else
  const auto statFunc = stat;
  typedef struct stat statStruct;
#endif

  auto path = absolutify().toString();
  auto cstrPath = path.c_str();
  statStruct buf;
  if (statFunc(cstrPath, &buf) != 0) return false;
  return buf.st_mode & S_IFDIR;
};
bool AltaCore::Filesystem::Path::isAbsolute() {
  return hasRoot;
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::operator /(const AltaCore::Filesystem::Path& rhs) {
  auto newPath = Path(*this);
  newPath.components.insert(newPath.components.end(), rhs.components.begin(), rhs.components.end());
  return newPath;
};
AltaCore::Filesystem::Path AltaCore::Filesystem::Path::operator /(const std::string& rhs) {
  return *this / Path(rhs);
};
AltaCore::Filesystem::Path AltaCore::Filesystem::Path::operator +(const std::string& rhs) {
  auto newPath = Path(*this);
  if (!newPath.hasComponents()) {
    newPath.components.push_back(rhs);
  } else {
    auto& base = newPath.components[newPath.components.size() - 1];
    base += rhs;
  }
  return newPath;
};
bool AltaCore::Filesystem::Path::operator ==(const AltaCore::Filesystem::Path& rhs) {
  if (hasRoot != rhs.hasRoot) return false;
  if (root != rhs.root) return false;
  if (components.size() != rhs.components.size()) return false;
  for (size_t i = 0; i < rhs.components.size(); i++) {
    if (components[i] != rhs.components[i]) return false;
  }
  return true;
};
AltaCore::Filesystem::Path::operator bool() {
  return isValid();
};
