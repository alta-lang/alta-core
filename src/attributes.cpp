#include "../include/altacore/attributes.hpp"
#include "../include/altacore/simple-map.hpp"

namespace AltaCore {
  namespace Attributes {
    std::vector<Attribute> registeredGlobalAttributes;
    ALTACORE_MAP<std::string, std::vector<Attribute>> registeredFileAttributes;
  };
};

AltaCore::Attributes::AttributeArgument::AttributeArgument(std::string _string):
  isString(true),
  string(_string)
  {};
AltaCore::Attributes::AttributeArgument::AttributeArgument(int _integer):
  isInteger(true),
  integer(_integer)
  {};
AltaCore::Attributes::AttributeArgument::AttributeArgument(bool _boolean):
  isBoolean(true),
  boolean(_boolean)
  {};
AltaCore::Attributes::AttributeArgument::AttributeArgument(std::shared_ptr<DET::ScopeItem> _item):
  isScopeItem(true),
  item(_item)
  {};
AltaCore::Attributes::AttributeArgument::AttributeArgument(double _decimal):
  isDecimal(true),
  decimal(_decimal)
  {};

AltaCore::Attributes::Attribute::Attribute(
  std::string _name,
  std::vector<AST::NodeType> _appliesTo,
  std::function<void(std::shared_ptr<AltaCore::AST::Node>, std::shared_ptr<DH::Node>, std::vector<AltaCore::Attributes::AttributeArgument>)> _callback,
  bool _isDomain
):
  name(_name),
  appliesTo(_appliesTo),
  isDomain(_isDomain),
  isGeneral(_appliesTo.size() == 0),
  callback(_callback)
  {};

bool AltaCore::Attributes::Attribute::checkIfAppliesTo(AltaCore::AST::NodeType type) {
  if (appliesTo.size() == 0) return true;
  for (auto& appl: appliesTo) {
    if (appl == type) return true;
  }
  return false;
};

bool AltaCore::Attributes::registerAttribute(std::vector<std::string> fullDomainPath, std::vector<AltaCore::AST::NodeType> appliesTo, std::function<void(std::shared_ptr<AltaCore::AST::Node>, std::shared_ptr<AltaCore::DH::Node>, std::vector<AltaCore::Attributes::AttributeArgument>)> callback, std::string file) {
  if (fullDomainPath.size() == 0) return false;

  std::vector<Attribute>* target = nullptr;

  if (file == "") {
    target = &registeredGlobalAttributes;
  } else {
    target = &registeredFileAttributes[file];
  }

  for (size_t i = 0; i < fullDomainPath.size() - 1; i++) {
    bool found = false;
    bool isNonDomain = false;
    for (auto& attribute: *target) {
      if (attribute.name == fullDomainPath[i]) {
        found = true;
        target = &attribute.children;
        if (!attribute.isDomain) {
          isNonDomain = true;
        }
        break;
      }
    }
    if (found && isNonDomain) {
      return false;
    } else if (!found) {
      target = &(target->emplace_back(fullDomainPath[i], std::vector<AST::NodeType> {}, nullptr, true).children);
    }
  }

  auto& last = fullDomainPath[fullDomainPath.size() - 1];
  for (auto& attribute: *target) {
    if (attribute.name == last) {
      bool applies = false;
      for (auto& app: appliesTo) {
        if (attribute.checkIfAppliesTo(app)) {
          applies = true;
          break;
        }
      }
      if (!applies) continue;
      return false;
    }
  }
  target->emplace_back(last, appliesTo, callback, false);

  bool isFirst = true;
  for (auto& item: fullDomainPath) {
    if (isFirst) {
      isFirst = false;
    } else {
      target->back().id += '.';
    }
    target->back().id += item;
  }

  return true;
};
ALTACORE_OPTIONAL<AltaCore::Attributes::Attribute> AltaCore::Attributes::findAttribute(std::vector<std::string> fullDomainPath, ALTACORE_OPTIONAL<AltaCore::AST::NodeType> appliesTo, std::string file) {
  if (fullDomainPath.size() == 0) return ALTACORE_NULLOPT;

  std::vector<Attribute>* target = nullptr;

  if (file == "") {
    target = &registeredGlobalAttributes;
  } else {
    target = &registeredFileAttributes[file];
  }

  for (size_t i = 0; i < fullDomainPath.size() - 1; i++) {
    bool found = false;

    for (auto& attr: *target) {
      if (attr.name == fullDomainPath[i]) {
        if (!attr.isDomain) return ALTACORE_NULLOPT;
        found = true;
        target = &attr.children;
        break;
      }
    }

    if (!found) return ALTACORE_NULLOPT;
  }

  auto& last = fullDomainPath[fullDomainPath.size() - 1];
  for (auto& attr: *target) {
    if (attr.name == last) {
      if (attr.isDomain) return ALTACORE_NULLOPT;
      if (appliesTo && !attr.checkIfAppliesTo(*appliesTo)) {
        continue;
      }
      return attr;
    }
  }

  if (file != "") {
    return findAttribute(fullDomainPath, appliesTo);
  }

  return ALTACORE_NULLOPT;
};

void AltaCore::Attributes::clearGlobalAttributes() {
  registeredGlobalAttributes.clear();
};
void AltaCore::Attributes::clearFileAttributes(std::string file) {
  if (registeredFileAttributes.find(file) != registeredFileAttributes.end()) {
    registeredFileAttributes[file].clear();
  }
};
void AltaCore::Attributes::clearAllAttributes() {
  registeredGlobalAttributes.clear();
  registeredFileAttributes.clear();
};
