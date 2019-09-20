#ifndef ALTACORE_ATTRIBUTES_HPP
#define ALTACORE_ATTRIBUTES_HPP

#include "simple-map.hpp"
#include "optional.hpp"
#include <vector>
#include <string>
#include <functional>
#include "ast-shared.hpp"
#include "det.hpp"

namespace AltaCore {
  namespace AST {
    class Node;
    class AttributeNode;
  };
  namespace DetailHandles {
    class Node;
    class AttributeNode;
  };
  namespace DH = DetailHandles;
  namespace Attributes {
    class AttributeArgument {
      public:
        bool isString = false;
        bool isInteger = false;
        bool isBoolean = false;
        bool isScopeItem = false;
        bool isDecimal = false;

        std::string string = "";
        int integer = 0;
        bool boolean = false;
        std::shared_ptr<DET::ScopeItem> item = nullptr;
        double decimal = 0;

        AttributeArgument(std::string string);
        AttributeArgument(int integer);
        AttributeArgument(bool boolean);
        AttributeArgument(std::shared_ptr<DET::ScopeItem> item);
        AttributeArgument(double decimal);
    };

    class Attribute {
      public:
        bool isDomain;
        bool isGeneral;
        std::string name;
        std::string id;
        std::vector<Attribute> children; // only available on attribute domains
        std::vector<AST::NodeType> appliesTo;
        bool postProcess = false;

        std::function<void(std::shared_ptr<AST::Node>, std::shared_ptr<DH::Node>, std::vector<AttributeArgument>)> callback = nullptr;

        bool checkIfAppliesTo(AST::NodeType type);

        Attribute(std::string name, std::vector<AST::NodeType> appliesTo, std::function<void(std::shared_ptr<AST::Node>, std::shared_ptr<DH::Node>, std::vector<AttributeArgument>)> callback = nullptr, bool isDomain = false, bool postProcess = false);
    };

    extern std::vector<Attribute> registeredGlobalAttributes;
    extern ALTACORE_MAP<std::string, std::vector<Attribute>> registeredFileAttributes;

    bool registerAttribute(std::vector<std::string> fullDomainPath, std::vector<AST::NodeType> appliesTo = {}, std::function<void(std::shared_ptr<AST::Node>, std::shared_ptr<DH::Node>, std::vector<AttributeArgument>)> callback = nullptr, std::string file = "", bool postProcess = false);
    ALTACORE_OPTIONAL<Attribute> findAttribute(std::vector<std::string> fullDomainPath, ALTACORE_OPTIONAL<AST::NodeType> appliesTo = ALTACORE_NULLOPT, std::string file = "");

    void clearGlobalAttributes();
    void clearFileAttributes(std::string file);
    void clearAllAttributes();

    std::vector<std::shared_ptr<DH::AttributeNode>> detailAttributes(std::vector<std::shared_ptr<AST::AttributeNode>>& attributes, std::shared_ptr<DET::Scope> scope, std::shared_ptr<AST::Node> ast, std::shared_ptr<DH::Node> info);
  };
};

#endif // ALTACORE_ATTRIBUTES_HPP
