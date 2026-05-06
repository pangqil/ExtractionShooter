#include "Enemy/AI/StateTree/PDStateTreeCondition_HasTarget.h"

#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "StateTreeExecutionContext.h"

bool FPDStateTreeCondition_HasTarget::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	return Combat && Combat->HasValidTarget();
}
