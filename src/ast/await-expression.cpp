#include "../../include/altacore/ast/await-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AwaitExpression::nodeType() {
	return NodeType::AwaitExpression;
};

ALTACORE_AST_DETAIL_D(AwaitExpression) {
	ALTACORE_MAKE_DH(AwaitExpression);

	info->target = target->fullDetail(info->inputScope);

	auto tgtType = DET::Type::getUnderlyingType(info->target.get());
	if (!tgtType->klass || tgtType->klass->name != "@Coroutine@") {
		ALTACORE_DETAILING_ERROR("`await` can only be performed on coroutine state instances");
	}

	auto func = Util::getFunction(info->inputScope).lock();

	//if (!func || !func->isAsync) {
	//	ALTACORE_DETAILING_ERROR("`await` can only be used inside coroutines");
	//}

	info->coroutine = func;

	return info;
};

ALTACORE_AST_VALIDATE_D(AwaitExpression) {
	ALTACORE_VS_S(AwaitExpression);

	target->validate(stack, info->target);

	ALTACORE_VS_E;
};
