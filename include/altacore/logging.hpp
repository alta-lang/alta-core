#ifndef ALTACORE_LOGGING_HPP
#define ALTACORE_LOGGING_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include <array>
#include "errors.hpp"
#include "simple-map.hpp"

namespace AltaCore {
  namespace Logging {
    using CodeSummary = std::pair<std::string, std::string>;
    constexpr auto make_summary = std::make_pair<std::string, std::string>;

    extern std::vector<std::pair<std::string, std::string>> shortSubsystemNames;
    extern ALTACORE_MAP<std::string, std::vector<CodeSummary>> codeSummaryRepositories;

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
        std::string _subsystem = "internal";
        std::string _code = "I0000";
        std::string _description;
        Errors::Position _location;

        Message(bool _internal, std::string subsystem, std::string code, Severity severity = Severity::Information, Errors::Position location = Errors::Position(), std::string description = ""):
          _subsystem(subsystem),
          _code(code),
          _severity(severity),
          _location(location),
          _description(description)
          {};

        void verifyCode(const std::string code) const;
        void verifySubsystem(const std::string subsystem) const;

      public:
        inline Severity severity() const {
          return _severity;
        }
        inline std::string code() const {
          return _code;
        };
        inline std::string subsystem() const {
          return _subsystem;
        };
        inline std::string description() const {
          return _description;
        };
        inline Errors::Position location() const {
          return _location;
        };
        std::string shortSubsystem() const;
        std::string summary() const;

        Message(std::string subsystem, const std::string code, Severity severity = Severity::Information, Errors::Position location = Errors::Position(), std::string description = ""):
          _subsystem(subsystem),
          _code(code),
          _severity(severity),
          _location(location),
          _description(description)
        {
          verifyCode(code);
          verifySubsystem(subsystem);
        };
        Message(std::string subsystem, const std::string code, Severity severity = Severity::Information, std::string description = ""):
          _subsystem(subsystem),
          _code(code),
          _severity(severity),
          _description(description)
        {
          verifyCode(code);
          verifySubsystem(subsystem);
        };
        Message(std::string subsystem, const std::string code, Errors::Position location = Errors::Position(), std::string description = ""):
          _subsystem(subsystem),
          _code(code),
          _location(location),
          _description(description)
        {
          verifyCode(code);
          verifySubsystem(subsystem);
        };
        Message(std::string subsystem, const std::string code, std::string description = ""):
          _subsystem(subsystem),
          _code(code),
          _description(description)
        {
          verifyCode(code);
          verifySubsystem(subsystem);
        };
    };
  };
};

#endif /* ALTACORE_LOGGING_HPP */
