#include "../../include/altacore/det/scope.hpp"
#include "../../include/altacore/det/function.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Scope::nodeType() {
  return NodeType::Scope;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Scope::clone() {
  return std::make_shared<Scope>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Scope::deepClone() {
  auto self = std::dynamic_pointer_cast<Scope>(clone());
  self->items.clear();
  for (auto& item: items) {
    auto newItem = std::dynamic_pointer_cast<ScopeItem>(item->deepClone());
    newItem->parentScope = self;
    self->items.push_back(newItem);
  }
  return self;
};

AltaCore::DET::Scope::Scope() {};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Scope> _parent):
  parent(_parent),
  relativeID(parent.lock()->nextChildID)
{
  parent.lock()->nextChildID++;
};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Module> _parentModule):
  parentModule(_parentModule)
  {};
AltaCore::DET::Scope::Scope(std::shared_ptr<AltaCore::DET::Function> _parentFunction):
  parentFunction(_parentFunction)
  {};

std::vector<std::shared_ptr<AltaCore::DET::ScopeItem>> AltaCore::DET::Scope::findAll(std::string name, std::vector<std::shared_ptr<Type>> excludeTypes) {
  std::vector<std::shared_ptr<ScopeItem>> results;
  std::vector<std::shared_ptr<Type>> funcTypes;
  std::shared_ptr<ScopeItem> first = nullptr;
  bool allFunctions = true;

  for (auto& item: items) {
    if (item->name == name) {
      if (item->nodeType() != NodeType::Function) {
        if (allFunctions && first) {
          // we've already found a non-function scope item with that name
          throw std::runtime_error("found non-function scope item with same name as another scope item; this is a conflict");
        } else if (allFunctions && results.size() == 0) {
          // we don't have a first item yet, and allFunctions is still true and there's no functions found.
          // this means we found the first (and hopefully only) item
          first = item;
        } else if (allFunctions) {
          // we've found functions with that name (since `results.size() > 0` here)
          throw std::runtime_error("found non-function scope item with same name as a function; this is a conflict");
        }
        allFunctions = false;
      } else if (!allFunctions) {
        // it is a function, but we already found a non-function item with that name
        throw std::runtime_error("found function scope item with same name as a non-function scope item; this is a conflict");
      } else {
        auto type = Type::getUnderlyingType(item);
        bool ok = true;
        if (excludeTypes.size() > 0) {
          for (auto& excl: excludeTypes) {
            if (type->isExactlyCompatibleWith(*excl)) {
              ok = false;
              break;
            }
          }
        }
        if (ok) {
          results.push_back(item);
          funcTypes.push_back(type);
        }
      }
    }
  }

  std::shared_ptr<Scope> parentScope = nullptr;
  if (!parent.expired()) {
    parentScope = parent.lock();
  } else if (!parentFunction.expired() && !parentFunction.lock()->parentScope.expired()) {
    parentScope = parentFunction.lock()->parentScope.lock();
  }

  if (parentScope != nullptr && (allFunctions || !first)) {
    // here, allFunctions being true means that either all the scope items found were functions
    // (in which case, we're free to search for overloads in parent scopes)
    // OR no items were found (which is fine, too, since we can also search parent scopes in that case)
    funcTypes.insert(funcTypes.end(), excludeTypes.begin(), excludeTypes.end());
    auto otherResults = parentScope->findAll(name, funcTypes);
    if (otherResults.size() == 1) {
      if (otherResults[0]->nodeType() == NodeType::Function) {
        results.push_back(otherResults[0]);
      } else if (!first) {
        first = otherResults[0];
      }
      // otherwise, ignore the result if it's not a function
    } else {
      results.insert(results.end(), otherResults.begin(), otherResults.end());
    }
  }

  if (first) {
    return { first };
  } else {
    return results;
  }
};

