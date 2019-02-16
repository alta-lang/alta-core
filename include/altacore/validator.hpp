#ifndef ALTACORE_VALIDATOR_HPP
#define ALTACORE_VALIDATOR_HPP

#include <memory>
#include <exception>
#include <string>

#include "fs.hpp"

namespace AltaCore {
  namespace AST {
    // forward declaration
    class Node;
  };
  namespace DetailHandles {
    class Node;
  };
  namespace DH = DetailHandles;
  namespace Validator {
    void validate(std::shared_ptr<AST::Node> target, std::shared_ptr<DH::Node> info = nullptr);
  };
};

#endif // ALTACORE_VALIDATOR_HPP
