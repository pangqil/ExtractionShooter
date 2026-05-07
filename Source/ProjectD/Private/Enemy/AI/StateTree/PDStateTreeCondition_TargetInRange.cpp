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

	// Chase(MoveToTarget) 의 도달 검사가 2D 거리이므로 본 Condition 도 2D 로 일관.
	// 3D 로 두면 Z 차이만큼 항상 더 멀어서 Chase 도달 직후 Combat 진입에 실패하는 토글 발생.
	const float DistSq = FVector::DistSquared2D(Pawn->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (Instance.Range * Instance.Range);
}
