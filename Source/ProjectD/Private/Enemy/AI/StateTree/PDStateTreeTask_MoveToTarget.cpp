#include "Enemy/AI/StateTree/PDStateTreeTask_MoveToTarget.h"

#include "AIController.h"
#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FPDStateTreeTask_MoveToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target)
	{
		// 타겟이 사라진 직후 진입했을 가능성 — 즉시 Failed 로 상위에 전이 위임.
		return EStateTreeRunStatus::Failed;
	}

	// MoveToActor 는 goal 이 움직이면 자동으로 path 갱신. 한 번만 호출.
	Instance.AIController->MoveToActor(Target, Instance.AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPDStateTreeTask_MoveToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat || !Combat->HasValidTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	const APawn* Pawn = Instance.AIController->GetPawn();
	const AActor* Target = Combat->GetCurrentTarget();
	if (!Pawn || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
	const float ThresholdSq = Instance.AcceptanceRadius * Instance.AcceptanceRadius;
	if (DistSq <= ThresholdSq)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FPDStateTreeTask_MoveToTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// Senior: 다음 Task 가 자유롭게 시작하도록 이동 명령을 명시적으로 정지.
	//         Combat Task 가 FocalPoint 만 잡고 사격해야 하는데 잔여 MoveTo 가 살아있으면 회전이 어긋남.
	if (Instance.AIController)
	{
		Instance.AIController->StopMovement();
	}
}
