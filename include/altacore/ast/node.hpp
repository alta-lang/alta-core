#ifndef ALTACORE_AST_NODE_HPP
#define ALTACORE_AST_NODE_HPP

#include "../ast-shared.hpp"
#include "../det/scope.hpp"
#include <stack>
#include <memory>
#include "../validator.hpp"

#define ALTACORE_AST_VALIDATE public: virtual void validate(ValidationStack& stack)
#define ALTACORE_AST_VALIDATE_D(x) void AltaCore::AST::x::validate(ValidationStack& stack)
// VS = validation stack
// S  = start
// E = end
#define ALTACORE_VS_S using AltaCore::Validator::ValidationError; stack.push(this)
#define ALTACORE_VS_E stack.pop()

namespace AltaCore {
  namespace AST {
    class Node {
        friend void AltaCore::Validator::validate(std::shared_ptr<Node>);

      public:
        using ValidationStack = std::stack<Node*>;

        virtual ~Node() = default;

        std::string id;

        Node();

        virtual const NodeType nodeType();

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_NODE_HPP
