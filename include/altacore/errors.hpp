#ifndef ALTACORE_ERRORS_HPP
#define ALTACORE_ERRORS_HPP

#include <functional>
#include "fs.hpp"

namespace AltaCore {
  namespace Errors {
    class Position {
      public:
        size_t line = 0;
        size_t column = 0;
        size_t filePosition = 0;
        Filesystem::Path file = Filesystem::Path();

        Position() {};
        Position(size_t _line, size_t _column, Filesystem::Path _file):
          line(_line),
          column(_column),
          file(_file)
          {};
    };

    class Error: public std::exception {
      private:
        std::string message;
      public:
        Position position;

        Error(std::string message = "", Position position = Position());
        const char* what() const noexcept override;
    };

    class DetailingError: public Error {
      public:
        DetailingError(std::string message = "", Position position = Position()):
          Error(message, position)
          {};
    };
    class ValidationError: public Error {
      public:
        ValidationError(std::string message = "", Position position = Position()):
          Error(message, position)
          {};
    };
    class ParsingError: public Error {
      public:
        ParsingError(std::string message = "", Position position = Position()):
          Error(message, position)
          {};
    };
    class LexingError: public Error {
      public:
        LexingError(std::string message = "", Position position = Position()):
          Error(message, position)
          {};
    };
  };
};

#endif /* ALTACORE_ERRORS_HPP */
