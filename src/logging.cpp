#include "../include/altacore/logging.hpp"

namespace AltaCore {
  namespace Logging {
    std::vector<Listener> listeners;
    std::vector<std::pair<std::string, std::string>> shortSubsystemNames = {
      { "parser", "PSR" },
      { "lexer", "LEX" },
      { "DET", "DET" },
      { "AST", "AST" },
      { "filesystem", "FS" },
      { "modules", "MOD" },
      { "validator", "VAL" },
      { "general", "GEN" },
      { "internal", "INT" },
    };

    ALTACORE_MAP<std::string, std::vector<CodeSummary>> codeSummaryRepositories = {
      { "parser", {
        make_summary("S0001", "Line starts with a parenthesis; may be incorrectly parsed as a function call"),
      } },
      { "lexer", {} },
      { "DET", {} },
      { "AST", {} },
      { "filesystem", {} },
      { "modules", {} },
      { "validator", {} },
      { "general", {
        make_summary("G0001", "Generic message - check description for more information"),
        make_summary("D0001", "Generic debug message"),
      } },
      { "internal", {
        make_summary("I0000", "Unknown internal message"),
        make_summary("I0001", "Message code is not 5 characters long"),
        make_summary("I0002", "Message code does not start with an uppercase letter (A-Z)"),
        make_summary("I0003", "Message code does not end with 4 digits (0-9)"),
        make_summary("I0004", "Subsystem not registered"),
      } },
    };
  };
};

size_t AltaCore::Logging::registerListener(Listener listener) {
  const auto idx = listeners.size();
  listeners.push_back(listener);
  return idx;
};

void AltaCore::Logging::log(Message message) {
  for (auto& listener: listeners) {
    listener(message);
  }
};

void AltaCore::Logging::Message::verifyCode(std::string code) const {
  if (code.size() != 5) {
    throw Message(true, "internal", "I0001", Severity::Error, Errors::Position(), "Code not 5 characters long\nCode is " + std::string(code));
  }
  if (code[0] < 'A' || code[0] > 'Z') {
    throw Message(true, "internal", "I0002", Severity::Error, Errors::Position(), "First character of code does not satisfy /[A-Z]/\nCode is " + std::string(code));
  }
  for (size_t i = 0; i < 4; ++i) {
    if (code[i + 1] < '0' || code[i + 1] > '9') {
      throw Message(true, "internal", "I0003", Severity::Error, Errors::Position(), "Last 4 characters of code are not digits\nCode is " + std::string(code));
    }
  }
};

void AltaCore::Logging::Message::verifySubsystem(std::string subsystem) const {
  for (auto& [longName, shortName]: shortSubsystemNames) {
    if (longName == subsystem)
      return;
  }
  throw Message(true, "internal", "I0004", Severity::Error, Errors::Position(), "Subsystem \"" + std::string(subsystem) + "\" not registered with the logging subsystem");
};

std::string AltaCore::Logging::Message::summary() const {
  if (codeSummaryRepositories.find(_subsystem) != codeSummaryRepositories.end()) {
    for (auto& entry: codeSummaryRepositories[_subsystem]) {
      if (entry.first == _code) {
        return std::string(entry.second);
      }
    }
  }
  for (auto& entry: codeSummaryRepositories["general"]) {
    if (entry.first == _code) {
      return std::string(entry.second);
    }
  }
  for (auto& entry: codeSummaryRepositories["internal"]) {
    if (entry.first == _code) {
      return std::string(entry.second);
    }
  }
  return "~~No summary available for code~~";
};

std::string AltaCore::Logging::Message::shortSubsystem() const {
  for (auto& [longName, shortName]: shortSubsystemNames) {
    if (longName == _subsystem)
      return shortName;
  }
  return _subsystem;
};
