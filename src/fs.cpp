#include "../include/altacore/fs.hpp"
#include <memory>
#include <functional>
#include <fstream>

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
#include <dirent.h>
#include <string.h>
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
  char tmp[MAXPATHLEN] = {0};
  return (getcwd(tmp, MAXPATHLEN) ? std::string(tmp) : std::string());
#endif
};

void AltaCore::Filesystem::copyFile(AltaCore::Filesystem::Path source, AltaCore::Filesystem::Path destination) {
  std::ifstream sourceStream(source.toString(), std::ios::binary);
  std::ofstream destinationStream(destination.toString(), std::ios::binary);

  // thank you, Martin York! see https://stackoverflow.com/a/10195497
  destinationStream << sourceStream.rdbuf();
};

std::vector<AltaCore::Filesystem::Path> AltaCore::Filesystem::getDirectoryListing(AltaCore::Filesystem::Path directory, bool recursive) {
  std::vector<Path> results;

#ifdef WIN32
  // https://docs.microsoft.com/en-us/windows/desktop/fileio/listing-the-files-in-a-directory
  WIN32_FIND_DATAW directoryFindData;
  HANDLE findHandle;

  auto dirStr = directory.toString();
  std::wstring dirWStr(dirStr.size(), L' ');
  std::copy(dirStr.begin(), dirStr.end(), dirWStr.begin());
  dirWStr += L"\\*";

  findHandle = FindFirstFileW(dirWStr.c_str(), &directoryFindData);

  if (findHandle == INVALID_HANDLE_VALUE) {
    throw std::runtime_error("invalid copy handle");
  }

  do {
    std::wstring wideFileName(directoryFindData.cFileName);
    if (wideFileName != L"." && wideFileName != L"..") {
      std::string narrowFileName(wideFileName.size(), ' ');
      std::copy(wideFileName.begin(), wideFileName.end(), narrowFileName.begin());
      results.push_back(Path(narrowFileName).absolutify(directory));
    }
  } while (FindNextFileW(findHandle, &directoryFindData) != 0);

  FindClose(findHandle);
#else
  auto stringifiedPath = directory.toString('/');

  // thanks, Peter Parker (https://stackoverflow.com/a/612176)
  DIR* dir = NULL;
  struct dirent* ent = NULL;
  if ((dir = opendir(stringifiedPath.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
        results.push_back(Path(ent->d_name).absolutify(directory));
      }
    }
    closedir(dir);
  } else {
    throw std::runtime_error("failed to open directory");
  }
#endif

  if (recursive) {
    for (auto& result: results) {
      if (result.isDirectory()) {
        auto arr = getDirectoryListing(result, true);
        results.insert(results.end(), arr.begin(), arr.end());
      }
    }
  }

  return results;
};

void AltaCore::Filesystem::copyDirectory(AltaCore::Filesystem::Path source, AltaCore::Filesystem::Path destination) {
  auto files = getDirectoryListing(source, true);
  mkdirp(destination);
  for (auto& file: files) {
    auto relPath = file.relativeTo(source);
    if (file.isDirectory()) {
      mkdirp(destination / relPath);
    } else {
      mkdirp(file.dirname());
      copyFile(file, destination / relPath);
    }
  }
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

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::normalize() const {
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

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::absolutify(AltaCore::Filesystem::Path relativeTo) const {
  // btw, we're free to modify `relativeTo` directly, since it's passed in by value
  auto newPath = Path(*this);
  newPath.absolutifyInPlace(relativeTo);
  return newPath;
};
AltaCore::Filesystem::Path AltaCore::Filesystem::Path::absolutify(std::string relativeTo) const {
  return absolutify(Path(relativeTo));
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::relativeTo(AltaCore::Filesystem::Path other) const {
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

std::string AltaCore::Filesystem::Path::toString(std::string separator) const {
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
std::string AltaCore::Filesystem::Path::toString(char separator) const {
  return toString(std::string(1, separator));
};

std::string AltaCore::Filesystem::Path::basename() const {
  if (!hasComponents()) return "";
  auto size = components.size();
  if (size < 1) return "";
  return components[size - 1];
};

std::string AltaCore::Filesystem::Path::filename() const {
  if (!hasComponents()) return "";
  auto base = basename();
  auto pos = base.find_last_of('.');
  if (pos == std::string::npos) return base;
  return base.substr(0, pos);
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::dirname() const {
  if (!hasComponents()) return Path();
  auto newPath = Path(*this);
  newPath.components.pop_back();
  return newPath;
};

std::string AltaCore::Filesystem::Path::extname() const {
  if (!hasComponents()) return "";
  auto base = basename();
  return base.substr(base.find_last_of('.') + 1); // doesn't include the '.'
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::uproot() const {
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

bool AltaCore::Filesystem::Path::isValid() const {
  return (hasRoot || components.size() > 0);
};
bool AltaCore::Filesystem::Path::isEmpty() const {
  return !isValid();
};
bool AltaCore::Filesystem::Path::isRoot() const {
  return (hasRoot && components.size() == 0);
};
bool AltaCore::Filesystem::Path::hasComponents() const {
  return components.size() > 0;
};
bool AltaCore::Filesystem::Path::exists() const {
  if (!isValid()) return false;

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
bool AltaCore::Filesystem::Path::isDirectory() const {
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
bool AltaCore::Filesystem::Path::isAbsolute() const {
  return hasRoot;
};

bool AltaCore::Filesystem::Path::hasParentDirectory(const AltaCore::Filesystem::Path& other) const {
  if (hasRoot != other.hasRoot) return false;
  if (root != other.root) return false;
  if (other.components.size() > components.size()) return false;
  for (size_t i = 0; i < other.components.size(); i++) {
    if (other.components[i] != components[i]) return false;
  }
  return true;
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::operator /(const AltaCore::Filesystem::Path& rhs) const {
  auto newPath = Path(*this);
  newPath.components.insert(newPath.components.end(), rhs.components.begin(), rhs.components.end());
  return newPath;
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::operator /(const std::string& rhs) const {
  return *this / Path(rhs);
};

AltaCore::Filesystem::Path AltaCore::Filesystem::Path::operator +(const std::string& rhs) const {
  auto newPath = Path(*this);
  if (!newPath.hasComponents()) {
    newPath.components.push_back(rhs);
  } else {
    auto& base = newPath.components[newPath.components.size() - 1];
    base += rhs;
  }
  return newPath;
};

bool AltaCore::Filesystem::Path::operator ==(const AltaCore::Filesystem::Path& rhs) const {
  if (hasRoot != rhs.hasRoot) return false;
  if (root != rhs.root) return false;
  if (components.size() != rhs.components.size()) return false;
  for (size_t i = 0; i < rhs.components.size(); i++) {
    if (components[i] != rhs.components[i]) return false;
  }
  return true;
};

AltaCore::Filesystem::Path::operator bool() const {
  return isValid();
};

bool AltaCore::Filesystem::Path::operator <(const AltaCore::Filesystem::Path& rhs) const {
  if (hasRoot != rhs.hasRoot && hasRoot) return false;

  if (hasRoot) {
    auto comp = root.compare(rhs.root);
    if (comp < 0) return true;
    if (comp > 0) return false;
  }

  if (components.size() > rhs.components.size()) return false;

  for (size_t i = 0; i < components.size(); i++) {
    auto& comp1 = components[i];
    auto& comp2 = rhs.components[i];

    auto comparison = comp1.compare(comp2);
    if (comparison < 0) return true;
    if (comparison > 0) return false;
  }

  // at this point, they're equal, so return false
  return false;
};

// create a `std::hash` specialization to allow `AltaCore::Filesystem::Path`s
// to be used as keys in `std::unordered_map`s
std::size_t std::hash<AltaCore::Filesystem::Path>::operator()(const AltaCore::Filesystem::Path& path) const {
  // effective hasing method according to
  // https://stackoverflow.com/a/17017281
  size_t res = 17;
  res = (res * 31) + hash<bool>()(path.hasRoot);
  res = (res * 31) + hash<std::string>()(path.root);
  for (auto& component: path.components) {
    res = (res * 31) + hash<std::string>()(component);
  }
  return res;
};
