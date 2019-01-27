#include "../include/altacore/validator.hpp"
#include "../include/altacore/ast.hpp"
#include "../include/altacore/det.hpp"
#include <stack>
#include <memory>

AltaCore::Validator::ValidationError::ValidationError(std::string _message) {
  message = _message;
};

const char* AltaCore::Validator::ValidationError::what() const noexcept {
  return message.c_str();
};

void AltaCore::Validator::validate(std::shared_ptr<AltaCore::AST::Node> target) {
  std::stack<AST::Node*> validationStack;
  return target->validate(validationStack);
};