#include "../../include/altacore/ast/class-definition-node.hpp"
#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"
#include "../../include/altacore/util.hpp"
#include "../../include/altacore/ast/super-class-fetch.hpp"
#include "../../include/altacore/ast/class-instantiation-expression.hpp"
#include "../../include/altacore/ast/integer-literal-node.hpp"

namespace AltaCoreClassHelpers {
  template<class T> void detailClass(std::shared_ptr<T> info, AltaCore::AST::ClassDefinitionNode* self) {
    using namespace AltaCore::AST;
    namespace DH = AltaCore::DetailHandles;
    namespace DET = AltaCore::DET;

    for (auto& parent: self->parents) {
      auto det = parent->fullDetail(info->klass->scope);
      info->parents.push_back(det);
      if (det->items.size() != 1 || det->items.back()->nodeType() != DET::NodeType::Class) {
        throw AltaCore::Errors::ValidationError("no class found for the given parent expression", self->position);
      }
      info->klass->parents.push_back(std::dynamic_pointer_cast<DET::Class>(det->items.back()));
    }

    auto loop = [&](std::vector<std::shared_ptr<ClassStatementNode>>& tgt, bool noBodies = false) -> void {
      if (noBodies) {
        for (auto stmt: tgt) {
          auto det = stmt->fullDetail(info->klass->scope, true);
          info->statements.push_back(det);
          if (stmt->nodeType() == NodeType::ClassSpecialMethodDefinitionStatement) {
            auto special = std::dynamic_pointer_cast<ClassSpecialMethodDefinitionStatement>(stmt);
            auto specialDet = std::dynamic_pointer_cast<DH::ClassSpecialMethodDefinitionStatement>(det);
            if (special->type == SpecialClassMethod::Constructor) {
              info->klass->constructors.push_back(specialDet->method);
              if (specialDet->method->parameters.size() == 0 || (specialDet->method->parameters.size() == 1 && std::get<2>(specialDet->method->parameters.front()))) {
                info->klass->defaultConstructor = specialDet->method;
              }
              if (specialDet->isCopyConstructor) {
                info->klass->copyConstructor = specialDet->method;
              }
            } else {
              if (info->klass->destructor) {
                throw AltaCore::Errors::ValidationError("can't have more than one destructor for a class", self->position);
              }
              info->klass->destructor = specialDet->method;
            }
          }
        }
      } else {
        for (size_t i = 0; i < tgt.size(); i++) {
          auto& stmt = tgt[i];
          auto& det = info->statements[i];
          stmt->detail(det, false);
        }
      }
    };

    loop(self->statements, true);
    loop(self->statements);

    if (info->klass->constructors.size() == 0) {
      info->createDefaultConstructor = true;
      info->defaultConstructor = std::make_shared<ClassSpecialMethodDefinitionStatement>(Visibility::Public, SpecialClassMethod::Constructor);
      info->defaultConstructor->body = std::make_shared<BlockNode>();
      for (size_t i = 0; i < info->klass->parents.size(); i++) {
        auto& parent = info->klass->parents[i];
        if (!parent->defaultConstructor) {
          throw AltaCore::Errors::ValidationError("at least one parent does not have a default constructor; one cannot be created automatically for this child class", self->position);
        }
        auto idx = std::make_shared<IntegerLiteralNode>(std::to_string(i));
        auto fetch = std::make_shared<SuperClassFetch>();
        fetch->fetch = idx;
        auto inst = std::make_shared<ClassInstantiationExpression>();
        inst->target = fetch;
        auto stmt = std::make_shared<ExpressionStatement>();
        stmt->expression = inst;
        info->defaultConstructor->body->statements.push_back(stmt);
      }
      info->defaultConstructorDetail = info->defaultConstructor->fullDetail(info->klass->scope);
      info->klass->constructors.push_back(info->defaultConstructorDetail->method);
      info->klass->defaultConstructor = info->defaultConstructorDetail->method;
    }

    bool requiresDtor = false;
    bool requiresCopyCtor = true;
    for (auto& item: info->klass->scope->items) {
      if (item->nodeType() != DET::NodeType::Variable) continue;
      if (item->name == "this") continue;

      auto var = std::dynamic_pointer_cast<DET::Variable>(item);

      if (var->type->isNative) continue;
      if (var->type->indirectionLevel() > 0) continue;

      if (var->type->isUnion()) {
        requiresDtor = true;
        requiresCopyCtor = true;
        info->klass->itemsToDestroy.push_back(var);
        info->klass->itemsToCopy.push_back(var);
      } else {
        if (var->type->klass->destructor) {
          requiresDtor = true;
          info->klass->itemsToDestroy.push_back(var);
        }

        if (var->type->klass->copyConstructor) {
          requiresCopyCtor = true;
          info->klass->itemsToCopy.push_back(var);
        }
      }
    }
    for (auto& parent: info->klass->parents) {
      if (parent->copyConstructor) {
        requiresCopyCtor = true;
      }
    }

    if (!info->klass->destructor && requiresDtor) {
      info->createDefaultDestructor = true;
      info->defaultDestructor = std::make_shared<ClassSpecialMethodDefinitionStatement>(Visibility::Public, SpecialClassMethod::Destructor);
      info->defaultDestructor->body = std::make_shared<BlockNode>();
      info->defaultDestructorDetail = info->defaultDestructor->fullDetail(info->klass->scope);
      info->klass->destructor = info->defaultDestructorDetail->method;
    }

    if (!info->klass->copyConstructor && requiresCopyCtor) {
      info->createDefaultCopyConstructor = true;
      info->defaultCopyConstructor = std::make_shared<ClassSpecialMethodDefinitionStatement>(Visibility::Public, SpecialClassMethod::Constructor);
      info->defaultCopyConstructor->body = std::make_shared<BlockNode>();

      // this is very hacky and i don't like it
      //
      // looks like my own API design came and bit
      // me in the butt
      auto selfType = std::make_shared<Type>();
      selfType->_injected_type = std::make_shared<DET::Type>(info->klass, std::vector<uint8_t> { (uint8_t)TypeModifierFlag::Reference });

      info->defaultCopyConstructor->parameters.push_back(std::make_shared<Parameter>("other", selfType));
      info->defaultCopyConstructorDetail = info->defaultCopyConstructor->fullDetail(info->klass->scope);
      info->defaultCopyConstructorDetail->isCopyConstructor = true;
      info->defaultCopyConstructorDetail->isDefaultCopyConstructor = true;
      info->klass->copyConstructor = info->defaultCopyConstructorDetail->method;
    }
  };
};

const AltaCore::AST::NodeType AltaCore::AST::ClassDefinitionNode::nodeType() {
  return NodeType::ClassDefinitionNode;
};

AltaCore::AST::ClassDefinitionNode::ClassDefinitionNode(std::string _name):
  name(_name)
  {};

ALTACORE_AST_DETAIL_D(ClassDefinitionNode) {
  ALTACORE_MAKE_DH(ClassDefinitionNode);

  info->klass = DET::Class::create(name, scope, {});
  info->klass->genericParameterCount = generics.size();
  info->klass->ast = shared_from_this();
  info->klass->info = info;

  for (auto generic: generics) {
    info->genericDetails.push_back(generic->fullDetail(info->klass->scope));
  }

  info->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  scope->items.push_back(info->klass);

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->klass);
    }
  }

  if (generics.size() > 0) {
    return info;
  }

  AltaCoreClassHelpers::detailClass(info, this);

  return info;
};

ALTACORE_AST_VALIDATE_D(ClassDefinitionNode) {
  ALTACORE_VS_S(ClassDefinitionNode);
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifier for class defintion");
  }
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for class definition");
  for (size_t i = 0; i < statements.size(); i++) {
    auto& stmt = statements[i];
    if (!stmt) ALTACORE_VALIDATION_ERROR("empty class statement in class definition");
    if (generics.size() > 0) {
      for (auto inst: info->genericInstantiations) {
        auto& stmtDet = inst->statements[i];
        stmt->validate(stack, stmtDet);
      }
    } else {
      auto& stmtDet = info->statements[i];
      stmt->validate(stack, stmtDet);
    }
  }
  if (!info->klass) ALTACORE_VALIDATION_ERROR("class defintion not detailed properly, no DET class attached");
  ALTACORE_VS_E;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::AST::ClassDefinitionNode::instantiateGeneric(std::shared_ptr<DH::ClassDefinitionNode> info, std::vector<std::shared_ptr<DET::Type>> genericArguments) {
  if (genericArguments.size() != generics.size()) {
    return nullptr;
  }

  for (auto genericInst: info->genericInstantiations) {
    bool ok = true;
    for (size_t i = 0; i < genericInst->genericDetails.size(); i++) {
      if (auto target = std::dynamic_pointer_cast<DET::Type>(genericInst->genericDetails[i]->alias->target)) {
        if (!target->isExactlyCompatibleWith(*genericArguments[i])) {
          ok = false;
          break;
        }
      } else {
        ok = false;
        break;
      }
    }

    if (!ok) {
      continue;
    }

    return genericInst->klass;
  }

  auto inst = std::make_shared<DH::GenericClassInstantiationDefinitionNode>(info->inputScope);
  info->genericInstantiations.push_back(inst);

  inst->klass = DET::Class::create(name, info->inputScope, {});
  inst->klass->ast = shared_from_this();
  inst->klass->info = info;
  inst->klass->genericParameterCount = info->klass->genericParameterCount;

  inst->klass->genericArguments = genericArguments;

  auto thisMod = Util::getModule(info->inputScope.get()).lock();
  auto& gDepEntry = thisMod->genericDependencies[inst->klass->id];

  for (size_t i = 0; i < generics.size(); i++) {
    auto& generic = generics[i];
    auto& genericArg = genericArguments[i];

    auto det = generic->fullDetail(inst->klass->scope);
    det->alias->target = genericArg;
    inst->genericDetails.push_back(det);

    if (genericArg->klass) {
      auto thatMod = Util::getModule(genericArg->klass->parentScope.lock().get()).lock();
      gDepEntry.push_back(thatMod);
    }
  }

  AltaCoreClassHelpers::detailClass(inst, this);

  return inst->klass;
};
