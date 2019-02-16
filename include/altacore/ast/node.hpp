#ifndef ALTACORE_AST_NODE_HPP
#define ALTACORE_AST_NODE_HPP

#include "../ast-shared.hpp"
#include "../det/scope.hpp"
#include <stack>
#include <memory>
#include "../validator.hpp"
#include "../fs.hpp"
#include "../detail-handles.hpp"
#include "../errors.hpp"

#define ALTACORE_AST_VALIDATE public: virtual void validate(ValidationStack& stack, std::shared_ptr<DH::Node> info)
#define ALTACORE_AST_VALIDATE_D(x) void AltaCore::AST::x::validate(ValidationStack& stack, std::shared_ptr<DH::Node> _info)

// VS = validation stack
// S  = start
// E  = end
// SS = simple start
#define ALTACORE_VS_SS stack.push(this)
#define ALTACORE_VS_S(x) stack.push(this); auto info = std::dynamic_pointer_cast<DH::x>(_info)
#define ALTACORE_VALIDATION_ERROR(x) throw AltaCore::Errors::ValidationError(x, position)
#define ALTACORE_VS_E stack.pop()

#define ALTACORE_DETAILING_ERROR(x) throw AltaCore::Errors::DetailingError(x, position)

#define ALTACORE_AST_DETAIL(x) public: virtual std::shared_ptr<AltaCore::DH::Node> detail(std::shared_ptr<AltaCore::DET::Scope> scope);\
  std::shared_ptr<AltaCore::DH::x> fullDetail(std::shared_ptr<AltaCore::DET::Scope> scope) {\
    return std::dynamic_pointer_cast<AltaCore::DH::x>(detail(scope));\
  }
#define ALTACORE_AST_MIN_DETAIL(x) std::shared_ptr<AltaCore::DH::x> fullDetail(std::shared_ptr<AltaCore::DET::Scope> scope) {\
    return std::dynamic_pointer_cast<AltaCore::DH::x>(detail(scope));\
  }
#define ALTACORE_AST_AUTO_DETAIL(x) virtual std::shared_ptr<AltaCore::DH::Node> detail(std::shared_ptr<AltaCore::DET::Scope> scope) {\
    ALTACORE_MAKE_DH(x);\
    return info;\
  };\
  std::shared_ptr<AltaCore::DH::x> fullDetail(std::shared_ptr<AltaCore::DET::Scope> scope) {\
    return std::dynamic_pointer_cast<AltaCore::DH::x>(detail(scope));\
  }
#define ALTACORE_AST_DETAIL_D(x) std::shared_ptr<AltaCore::DH::Node> AltaCore::AST::x::detail(std::shared_ptr<AltaCore::DET::Scope> scope)

// DH = detail handle
#define ALTACORE_MAKE_DH(x) auto info = std::make_shared<DH::x>(scope);

namespace AltaCore {
  namespace AST {
    using Errors::Position;
    
    class Node {
        friend void AltaCore::Validator::validate(std::shared_ptr<Node>, std::shared_ptr<DH::Node>);

      public:
        using ValidationStack = std::stack<Node*>;

        virtual ~Node() = default;

        std::string id;
        Position position;

        Node();

        virtual const NodeType nodeType();

        ALTACORE_AST_DETAIL(Node);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_NODE_HPP
