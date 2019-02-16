#include "../include/altacore/validator.hpp"
#include "../include/altacore/ast.hpp"
#include "../include/altacore/det.hpp"
#include <stack>
#include <memory>

void AltaCore::Validator::validate(std::shared_ptr<AltaCore::AST::Node> target, std::shared_ptr<AltaCore::DH::Node> info) {
  std::stack<AST::Node*> validationStack;
  return target->validate(validationStack, info);
};
