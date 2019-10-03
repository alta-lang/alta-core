#include "../../include/altacore/ast/try-catch-block.hpp"

const AltaCore::AST::NodeType AltaCore::AST::TryCatchBlock::nodeType() {
  return NodeType::TryCatchBlock;
};

ALTACORE_AST_DETAIL_D(TryCatchBlock) {
  ALTACORE_MAKE_DH(TryCatchBlock);
  info->tryScope = DET::Scope::makeWithParentScope(info->inputScope);
  info->tryScope->isTry = true;
  info->tryBlock = tryBlock->fullDetail(info->tryScope);
  std::unordered_set<std::shared_ptr<DET::Type>, DET::TypePointerHash, DET::TypePointerComparator> caught;
  for (size_t i = 0; i < catchBlocks.size(); i++) {
    auto& id = catchIDs[i];
    auto& [type, block] = catchBlocks[i];
    auto scope = DET::Scope::makeWithParentScope(info->inputScope);
    info->catchScopes.push_back(scope);
    auto typeDet = type->fullDetail(scope);
    auto err = typeDet->type->copy()->deconstify();
    caught.insert(err);
    auto errorVar = std::make_shared<DET::Variable>(id, typeDet->type, scope);
    info->errorVariables.push_back(errorVar);
    scope->items.push_back(errorVar);
    auto stmtDet = block->fullDetail(scope);
    info->catchBlocks.push_back(std::make_pair(typeDet, stmtDet));
  }
  if (catchAllBlock) {
    info->catchAllScope = DET::Scope::makeWithParentScope(info->inputScope);
    info->catchAllBlock = catchAllBlock->fullDetail(info->catchAllScope);
  } else {
    for (auto& type: info->tryScope->typesThrown) {
      auto typeHash = std::hash<DET::Type>()(*type);
      bool found = false;
      for (auto& item: caught) {
        if (std::hash<DET::Type>()(*item) == typeHash) {
          found = true;
          break;
        }
        if (!type->isNative && !item->isNative && type->klass && type->klass->hasParent(item->klass)) {
          found = true;
          break;
        }
      }
      if (!found) {
        info->inputScope->addPossibleError(type);
      }
    }
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(TryCatchBlock) {
  ALTACORE_VS_S(TryCatchBlock);
  if (catchBlocks.size() < 1 && !catchAllBlock) {
    ALTACORE_VALIDATION_ERROR("try-catch blocks must have at least one catch clause");
  }
  ALTACORE_VS_E;
};
