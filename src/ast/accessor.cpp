#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/det.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Accessor::nodeType() {
  return NodeType::Accessor;
};

AltaCore::AST::Accessor::Accessor(std::shared_ptr<AltaCore::AST::ExpressionNode> _target, std::string _query):
  target(_target),
  query(_query)
  {};

ALTACORE_AST_DETAIL_D(Accessor) {
  ALTACORE_MAKE_DH(Accessor);

  info->target = target->fullDetail(scope);

  std::shared_ptr<DET::Scope> targetScope = nullptr;
  auto targetAcc = std::dynamic_pointer_cast<Accessor>(target);
  auto targetAccDH = std::dynamic_pointer_cast<DH::Accessor>(info->target);

  if (targetAcc && targetAccDH->readAccessor) {
    if (targetAccDH->readAccessor->returnType->isNative) {
      throw std::runtime_error("native types can't be accessed");
    }
    targetScope = targetAccDH->readAccessor->returnType->klass->scope;
  } else {
    auto items = DET::ScopeItem::getUnderlyingItems(info->target);

    if (items.size() == 1) {
      if (items[0]->nodeType() == DET::NodeType::Function) {
        throw std::runtime_error("can't access a function");
      } else if (items[0]->nodeType() == DET::NodeType::Namespace) {
        info->accessesNamespace = true;
      }
      try {
        info->targetType = DET::Type::getUnderlyingType(items[0]);
      } catch (...) {
        info->targetType = nullptr;
      }
      targetScope = DET::Scope::getMemberScope(items[0]);
    } else if (items.size() > 0) {
      throw std::runtime_error("target must be narrowed before it can be accessed");
    } else {
      try {
        auto types = DET::Type::getUnderlyingTypes(info->target.get());
        if (types.size() == 1) {
          info->targetType = types[0];
          if (types[0]->isNative) {
            throw std::runtime_error("native types can't be accessed");
          }
          targetScope = types[0]->klass->scope;
        } else if (items.size() > 0) {
          throw std::runtime_error("target must be narrowed before it can be accessed");
        } else {
          // the `!targetScope` check will take care of this later
        }
      } catch (...) {
        // do nothing
      }
    }
  }

  if (!targetScope) {
    throw std::runtime_error("could not determine how to access the given target");
  }

  if (!targetScope->parentNamespace.expired()) {
    info->accessesNamespace = true;
  }

  info->items = targetScope->findAll(query, {}, false, scope);

  bool allAccessors = true;

  for (auto& item: info->items) {
    if (item->nodeType() == DET::NodeType::Function && std::dynamic_pointer_cast<DET::Function>(item)->isAccessor) {
      auto acc = std::dynamic_pointer_cast<DET::Function>(item);
      if (acc->parameters.size() == 0) {
        if (info->readAccessor) throw std::runtime_error("encountered two read accessors with the same name");
        info->readAccessor = acc;
      } else if (acc->parameters.size() == 1) {
        if (info->writeAccessor) throw std::runtime_error("encountered two write accessors with the same name");
        info->writeAccessor = acc;
      } else {
        throw std::runtime_error("invalid accessor");
      }
    }
  }

  if (info->items.size() == 0) {
    throw std::runtime_error("no items found for query in target");
  } else if (info->items.size() == 1) {
    if (info->items[0]->nodeType() != DET::NodeType::Function || !std::dynamic_pointer_cast<DET::Function>(info->items[0])->isAccessor) {
      info->narrowedTo = info->items[0];
    }
  }

  return info;
};

void AltaCore::AST::Accessor::narrowTo(std::shared_ptr<DH::Accessor> info, std::shared_ptr<AltaCore::DET::Type> type) {
  size_t highestCompat = 0;
  for (auto& item: info->items) {
    auto itemType = DET::Type::getUnderlyingType(item);
    auto compat = itemType->compatiblity(*type);
    if (compat > highestCompat) {
      if (item->nodeType() != DET::NodeType::Function || !std::dynamic_pointer_cast<DET::Function>(item)->isAccessor) {
        highestCompat = compat;
        info->narrowedTo = item;
      }
    }
  }
};

ALTACORE_AST_VALIDATE_D(Accessor) {
  ALTACORE_VS_S(Accessor);
  target->validate(stack, info->target);
  if (query.empty()) {
    ALTACORE_VALIDATION_ERROR("accessor query can't be empty");
  }
  ALTACORE_VS_E;
};
