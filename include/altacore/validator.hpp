#ifndef ALTACORE_VALIDATOR_HPP
#define ALTACORE_VALIDATOR_HPP

#include <memory>
#include <exception>
#include <string>

#include "fs.hpp"

namespace AltaCore {
  namespace AST {
    // forward declaration
    class Node;
  };
  namespace Validator {
    class ValidationError: public std::exception {
      private:
        std::string message;
    public:
        size_t line;
        size_t column;
        Filesystem::Path file;
      public:
        ValidationError(std::string message = "", size_t line = 1, size_t column = 1, Filesystem::Path file = Filesystem::Path());
        const char* what() const noexcept override;
    };
    void validate(std::shared_ptr<AST::Node> target);
  };
};

#endif // ALTACORE_VALIDATOR_HPP
