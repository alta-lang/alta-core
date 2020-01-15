#ifndef ALTACORE_LOGGING_HPP
#define ALTACORE_LOGGING_HPP

#include <cstddef>
#include <functional>
#include <string>
#include "errors.hpp"

namespace AltaCore {
  namespace Logging {
    /**
     * Indicates the severity of a message
     *
     * Ordered from least to most severe
     * (i.e. Verbose is least severe, Error is most severe)
     */
    enum class Severity {
      Verbose = 0,
      Information = 1,
      Warning = 2,
      Error = 3,
    };

    class Message;
    using Listener = std::function<void(Message)>;

    size_t registerListener(Listener listener);
    void log(Message message);

    class Message {
      private:
        Severity _severity = Severity::Information;
        std::string_view _code = "I0000";
        std::string _description;
        Errors::Position _location;

        Message(bool _internal, std::string_view code, Severity severity = Severity::Information, Errors::Position location = Errors::Position(), std::string description = ""):
          _code(code),
          _severity(severity),
          _location(location),
          _description(description)
          {};

        void verifyCode(const std::string_view code) const {
          if (_code.size() != 5) {
            throw Message(true, "I0001", Severity::Error, Errors::Position(), "Code not 5 characters long: " + std::string(_code));
          }
          if (_code[0] < 'A' || _code[0] > 'Z') {
            throw Message(true, "I0002", Severity::Error, Errors::Position(), "First character of code does not satisfy /[A-Za-z]/\nCode is " + std::string(_code));
          }
          for (size_t i = 0; i < 4; ++i) {
            if (_code[i + 1] < '0' || _code[i + 1] > '9') {
              throw Message(true, "I0003", Severity::Error, Errors::Position(), "Last 4 characters of code are not digits\nCode is " + std::string(_code));
            }
          }
        };

      public:
        inline Severity severity() const {
          return _severity;
        }
        inline std::string code() const {
          return std::string(_code);
        };
        inline std::string summary() const {
          for (auto& entry: Errors::messageCodeSummary) {
            if (entry.first == _code) {
              return std::string(entry.second);
            }
          }
          return "~~No summary available for code~~";
        };
        inline std::string description() const {
          return _description;
        };
        inline Errors::Position location() const {
          return _location;
        }

        Message(const std::string_view code, Severity severity = Severity::Information, Errors::Position location = Errors::Position(), std::string description = ""):
          _code(code),
          _severity(severity),
          _location(location),
          _description(description)
        {
          verifyCode(code);
        };
        Message(const std::string_view code, Severity severity = Severity::Information, std::string description = ""):
          _code(code),
          _severity(severity),
          _description(description)
        {
          verifyCode(code);
        };
        Message(const std::string_view code, Errors::Position location = Errors::Position(), std::string description = ""):
          _code(code),
          _location(location),
          _description(description)
        {
          verifyCode(code);
        };
        Message(const std::string_view code, std::string description = ""):
          _code(code),
          _description(description)
        {
          verifyCode(code);
        };
    };
  };
};

#endif /* ALTACORE_LOGGING_HPP */
