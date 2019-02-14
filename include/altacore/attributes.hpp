#ifndef ALTACORE_ATTRIBUTES_HPP
#define ALTACORE_ATTRIBUTES_HPP

#if defined(__has_include) && __has_include(<optional>)
#include <optional>
#define ALTACORE_OPTIONAL std::optional
#define ALTACORE_NULLOPT std::nullopt
#else
#include <optional.hpp>
#define ALTACORE_OPTIONAL tl::optional
#define ALTACORE_NULLOPT tl::nullopt
#endif

#include <vector>
#include <string>
#include <map>
#include <functional>
#include "ast-shared.hpp"

namespace AltaCore {
  namespace AST {
    class Node;
  };
  namespace DetailHandles {
    class Node;
  };
  namespace DH = DetailHandles;
  namespace Attributes {
    class AttributeArgument {
      public:
        bool isString = false;
        bool isInteger = false;
        bool isBoolean = false;

        std::string string;
        int integer = 0;
        bool boolean = false;

        AttributeArgument(std::string string);
        AttributeArgument(int integer);
        AttributeArgument(bool boolean);
    };

    class Attribute {
      public:
        bool isDomain;
        bool isGeneral;
        std::string name;
        std::string id;
        std::vector<Attribute> children; // only available on attribute domains
        std::vector<AST::NodeType> appliesTo;

        std::function<void(std::shared_ptr<AST::Node>, std::shared_ptr<DH::Node>, std::vector<AttributeArgument>)> callback = nullptr;

        bool checkIfAppliesTo(AST::NodeType type);

        Attribute(std::string name, std::vector<AST::NodeType> appliesTo, std::function<void(std::shared_ptr<AST::Node>, std::shared_ptr<DH::Node>, std::vector<AttributeArgument>)> callback = nullptr, bool isDomain = false);
    };

    extern std::vector<Attribute> registeredGlobalAttributes;
    extern std::map<std::string, std::vector<Attribute>> registeredFileAttributes;

    bool registerAttribute(std::vector<std::string> fullDomainPath, std::vector<AST::NodeType> appliesTo = {}, std::function<void(std::shared_ptr<AST::Node>, std::shared_ptr<DH::Node>, std::vector<AttributeArgument>)> callback = nullptr, std::string file = "");
    ALTACORE_OPTIONAL<Attribute> findAttribute(std::vector<std::string> fullDomainPath, ALTACORE_OPTIONAL<AST::NodeType> appliesTo = ALTACORE_NULLOPT, std::string file = "");

    void clearGlobalAttributes();
    void clearFileAttributes(std::string file);
    void clearAllAttributes();
  };
};

#endif // ALTACORE_ATTRIBUTES_HPP
