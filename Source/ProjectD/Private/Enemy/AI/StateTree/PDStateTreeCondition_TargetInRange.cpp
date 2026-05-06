#include "Enemy/AI/StateTree/PDStateTreeCondition_TargetInRange.h"

#include "AIController.h"
#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "StateTreeExecutionContext.h"

bool FPDStateTreeCondition_TargetInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat)
	{
		return false;
	}

	const AActor* Target = Combat->GetCurrentTarget();
	const APawn* Pawn = Instance.AIController ? Instance.AIController->GetPawn() : nullptr;
	if (!Target || !Pawn)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (Instance.Range * Instance.Range);
}
