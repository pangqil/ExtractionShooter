#include "Enemy/AI/StateTree/PDStateTreeCondition_HasNoiseHint.h"

#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "StateTreeExecutionContext.h"

bool FPDStateTreeCondition_HasNoiseHint::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	return Combat && Combat->HasNoiseHint();
}
