#ifndef ALTACORE_VALIDATOR_HPP
#define ALTACORE_VALIDATOR_HPP

#include <memory>
#include <exception>
#include <string>

namespace AltaCore {
  namespace AST {
    // forward declaration
    class Node;
  };
  namespace Validator {
    class ValidationError: public std::exception {
      private:
        std::string message;
      public:
        ValidationError(std::string message = "");
        const char* what() override;
    };
    void validate(std::shared_ptr<AST::Node> target);
  };
};

#endif // ALTACORE_VALIDATOR_HPP
