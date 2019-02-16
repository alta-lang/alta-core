#include "../include/altacore/errors.hpp"

AltaCore::Errors::Error::Error(std::string _message, Position _position):
  message(_message),
  position(_position)
  {};

const char* AltaCore::Errors::Error::what() const noexcept {
  return message.c_str();
};
