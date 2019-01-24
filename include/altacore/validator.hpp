#ifndef ALTACORE_VALIDATOR_HPP
#define ALTACORE_VALIDATOR_HPP

#include <memory>

namespace AltaCore {
  namespace AST {
    // forward declaration
    class Node;
  };
  namespace Validator {
    bool validate(std::shared_ptr<AST::Node> target);
  };
};

#endif // ALTACORE_VALIDATOR_HPP
