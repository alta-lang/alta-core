#include "../include/altacore/validator.hpp"
#include "../include/altacore/ast.hpp"
#include "../include/altacore/det.hpp"
#include <stack>
#include <memory>

AltaCore::Validator::ValidationError::ValidationError(std::string _message) {
  message = _message;
};

const char* AltaCore::Validator::ValidationError::what() {
  return message.c_str();
};

void AltaCore::Validator::validate(std::shared_ptr<AltaCore::AST::Node> target) {
  std::stack<std::shared_ptr<AST::Node>> validationStack;
  return target->validate(validationStack);
};