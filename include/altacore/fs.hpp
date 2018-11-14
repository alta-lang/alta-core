#ifndef ALTACORE_FS_HPP
#define ALTACORE_FS_HPP

#include <string>
#include <vector>

// why are we rolling our own filesystem implementation?
// why not use C++17's `std::filesystem`? alas, while i
// would like to use it, compiler/toolchain support is
// pretty sketchy (as of October 21, 2018, that is).
// besides, i guess this is good practice for how to implement
// a `filesystem` module in Alta

namespace AltaCore {
  namespace Filesystem {
    /**
     * The platform default separator
     */
    static const char platformSeparator = 
#if defined(_WIN32) || defined (_WIN64)
      '\\';
#else
      '/';
#endif

    std::string cwd();

    class Path {
      private:
        bool hasRoot = false;
        std::string root;

        void absolutifyInPlace(Path relativeTo);
      public:
        std::vector<std::string> components;

        Path();
        Path(std::string path, std::vector<std::string> separators = { "/", "\\" });
        Path(std::string path, std::string separator);

        Path normalize();
        Path absolutify(Path relativeTo);
        /**
         * @brief Create an absolute path from this path
         * By default, it is resolved relative to the current working directory
         */
        Path absolutify(std::string relativeTo = cwd());
        Path relativeTo(Path other);
        std::string toString(std::string separator = std::string(1, platformSeparator));
        std::string toString(char separator);
        std::string basename();
        std::string filename();
        Path dirname();
        /**
         * @brief Get the extname of this path
         * 
         * @return std::string The extension of this path's base (i.e. 'txt' in 'foo/bar/quux.txt')
         */
        std::string extname();
        /**
         * @brief Get a copy of this path, but without a root
         * 
         * @return Path The new, uprooted/rootless path
         */
        Path uproot();

        void push(std::string component);
        std::string pop();
        void unshift(std::string component);
        std::string shift();
        
        bool isValid();
        bool isEmpty();
        bool isRoot();
        bool hasComponents();
        bool exists();
        bool isDirectory();
        bool isAbsolute();

        Path operator /(const Path& rhs);
        Path operator /(const std::string& rhs);
        Path operator +(const std::string& rhs);
    };
    
    [[deprecated("Use `path.exists()` instead")]] bool exists(Path path);
    bool mkdirp(Path);
  };
};

#endif // ALTACORE_FS_HPP
