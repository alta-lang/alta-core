#ifndef ALTACORE_ERRORS_HPP
#define ALTACORE_ERRORS_HPP

#include <functional>
#include "fs.hpp"
#include <stdexcept>
#include <array>

namespace AltaCore {
  namespace Errors {
    namespace {
      template<typename... Args>
      constexpr std::array<typename std::common_type<Args...>::type, sizeof...(Args)> make_array(Args&&... args) {
        return { std::forward<Args>(args)... };
      };

      using CodeSummary = std::pair<std::string_view, std::string_view>;
      constexpr auto make_summary = std::make_pair<std::string_view, std::string_view>;
    };

    static constexpr auto messageCodeSummary = make_array(
      make_summary("S0001", "Line starts with a parenthesis; may be incorrectly parsed as a function call"),
      make_summary("I0001", "Message code is not 5 characters long"),
      make_summary("I0002", "Message code does not start with an uppercase letter (A-Z)"),
      make_summary("I0003", "Message code does not end with 4 digits (0-9)"),
      make_summary("G0001", "Generic message - check description for more information")
    );

    class Position {
      public:
        size_t line = 0;
        size_t column = 0;
        size_t filePosition = 0;
        Filesystem::Path file = Filesystem::Path();

        Position() {};
        Position(size_t _line, size_t _column, Filesystem::Path _file, size_t _filePosition = 0):
          line(_line),
          column(_column),
          file(_file),
          filePosition(_filePosition)
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
