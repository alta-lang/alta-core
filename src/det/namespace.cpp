#include "../../include/altacore/det/namespace.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Namespace::nodeType() {
  return NodeType::Namespace;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Namespace::clone() {
  return std::make_shared<Namespace>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Namespace::deepClone() {
  auto self = std::dynamic_pointer_cast<Namespace>(clone());
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());
  return self;
};

std::shared_ptr<AltaCore::DET::Namespace> AltaCore::DET::Namespace::create(std::shared_ptr<AltaCore::DET::Scope> parentScope, std::string name) {
  auto ns = std::make_shared<Namespace>(name, parentScope);
  ns->scope = std::make_shared<Scope>(ns);

  return ns;
};

AltaCore::DET::Namespace::Namespace(
  std::string _name,
  std::shared_ptr<AltaCore::DET::Scope> _parentScope
):
  ScopeItem(_name, _parentScope)
  {};

auto AltaCore::DET::Namespace::fullPrivateHoistedItems() const -> std::vector<std::shared_ptr<ScopeItem>> {
  auto result = privateHoistedItems;

  for (auto& item: scope->items) {
    auto priv = item->fullPrivateHoistedItems();
    result.insert(result.end(), priv.begin(), priv.end());
  }

  return result;
};
