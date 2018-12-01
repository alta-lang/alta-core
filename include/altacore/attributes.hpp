#ifndef ALTACORE_ATTRIBUTES_HPP
#define ALTACORE_ATTRIBUTES_HPP

#include <vector>
#include <string>
#include <optional>
#include <map>
#include <functional>
#include "ast-shared.hpp"
#include "ast/node.hpp"

namespace AltaCore {
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
        std::vector<Attribute> children; // only available on attribute domains
        std::vector<AST::NodeType> appliesTo;

        std::function<void(std::shared_ptr<AST::Node>, std::vector<AttributeArgument>)> callback = nullptr;

        bool checkIfAppliesTo(AST::NodeType type);

        Attribute(std::string name, std::vector<AST::NodeType> appliesTo, std::function<void(std::shared_ptr<AST::Node>, std::vector<AttributeArgument>)> callback = nullptr, bool isDomain = false);
    };

    extern std::vector<Attribute> registeredGlobalAttributes;
    extern std::map<std::string, std::vector<Attribute>> registeredFileAttributes;

    bool registerAttribute(std::vector<std::string> fullDomainPath, std::vector<AST::NodeType> appliesTo = {}, std::function<void(std::shared_ptr<AST::Node>, std::vector<AttributeArgument>)> callback = nullptr, std::string file = "");
    std::optional<Attribute> findAttribute(std::vector<std::string> fullDomainPath, std::optional<AST::NodeType> appliesTo = std::nullopt, std::string file = "");
  };
};

#endif // ALTACORE_ATTRIBUTES_HPP
