#include "../../include/altacore/ast/class-definition-node.hpp"
#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassDefinitionNode::nodeType() {
  return NodeType::ClassDefinitionNode;
};

AltaCore::AST::ClassDefinitionNode::ClassDefinitionNode(std::string _name):
  name(_name)
  {};

ALTACORE_AST_DETAIL_D(ClassDefinitionNode) {
  ALTACORE_MAKE_DH(ClassDefinitionNode);

  std::vector<std::shared_ptr<DET::Class>> parentClasses;
  for (auto& parent: parents) {
    auto det = parent->fullDetail(scope);
    info->parents.push_back(det);
    if (det->items.size() != 1 || det->items.back()->nodeType() != DET::NodeType::Class) {
      ALTACORE_DETAILING_ERROR("no class found for the given parent expression");
    }
    parentClasses.push_back(std::dynamic_pointer_cast<DET::Class>(det->items.back()));
  }

  info->klass = DET::Class::create(name, scope, parentClasses);
  scope->items.push_back(info->klass);

  info->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->klass);
    }
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
              ALTACORE_VALIDATION_ERROR("can't have more than one destructor for a class");
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

  loop(statements, true);
  loop(statements);

  if (info->klass->constructors.size() == 0) {
    info->createDefaultConstructor = true;
    info->defaultConstructor = std::make_shared<ClassSpecialMethodDefinitionStatement>(Visibility::Public, SpecialClassMethod::Constructor);
    info->defaultConstructor->body = std::make_shared<BlockNode>();
    info->defaultConstructorDetail = info->defaultConstructor->fullDetail(info->klass->scope);
    info->klass->constructors.push_back(info->defaultConstructorDetail->method);
  }

  bool requiresDtor = false;
  bool requiresCopyCtor = false;
  for (auto& item: info->klass->scope->items) {
    if (item->nodeType() != DET::NodeType::Variable) continue;
    if (item->name == "this") continue;

    auto var = std::dynamic_pointer_cast<DET::Variable>(item);

    if (var->type->isNative) continue;
    if (var->type->indirectionLevel() > 0) continue;

    if (var->type->klass->destructor) {
      requiresDtor = true;
      info->klass->itemsToDestroy.push_back(var);
    }

    if (var->type->klass->copyConstructor) {
      requiresCopyCtor = true;
      info->klass->itemsToCopy.push_back(var);
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
    auto& stmtDet = info->statements[i];
    if (!stmt) ALTACORE_VALIDATION_ERROR("empty class statement in class definition");
    stmt->validate(stack, stmtDet);
  }
  if (!info->klass) ALTACORE_VALIDATION_ERROR("class defintion not detailed properly, no DET class attached");
  ALTACORE_VS_E;
};
