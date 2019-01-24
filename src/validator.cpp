#include "../include/altacore/validator.hpp"
#include "../include/altacore/ast.hpp"
#include "../include/altacore/det.hpp"
#include <stack>
#include <memory>

bool AltaCore::Validator::validate(std::shared_ptr<AltaCore::AST::Node> target) {
  std::stack<std::shared_ptr<AST::Node>> validationStack;
  return target->validate(validationStack);
};