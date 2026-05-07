#include "Enemy/AI/StateTree/PDStateTreeTask_MoveToTarget.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FPDStateTreeTask_MoveToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat)
	{
		UE_LOG(LogPDAI, Verbose, TEXT("[MoveToTarget] EnterState Failed: Combat=null"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target)
	{
		// 타겟이 사라진 직후 진입했을 가능성 — 즉시 Failed 로 상위에 전이 위임.
		UE_LOG(LogPDAI, Verbose, TEXT("[MoveToTarget] EnterState Failed: Target=null"));
		return EStateTreeRunStatus::Failed;
	}

	// Target 이 공중/벽 안에 있어도 navmesh 위 도달 가능 점으로 보정해서 MoveToLocation 발행.
	// (MoveToActor 는 actor 위치를 그대로 사용해 navmesh 외 위치는 path 못 잡음. Tick 에서 수동 reissue 로 추적.)
	const FVector RawTargetLoc = Target->GetActorLocation();
	const FVector NavTargetLoc = PDStateTreeUtil::ProjectToNav(Instance.AIController, RawTargetLoc);

	const EPathFollowingRequestResult::Type Result =
		Instance.AIController->MoveToLocation(NavTargetLoc, Instance.AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true);

	const APawn* OwnerPawn = Instance.AIController->GetPawn();
	const float CurrentDist2D = OwnerPawn ? FVector::Dist2D(OwnerPawn->GetActorLocation(), NavTargetLoc) : -1.f;

	UE_LOG(LogPDAI, Log, TEXT("[MoveToTarget] EnterState: Target=%s, RawLoc=%s, NavLoc=%s, Dist2D=%.1f, AcceptanceRadius=%.1f, MoveTo=%s"),
		*GetNameSafe(Target), *RawTargetLoc.ToString(), *NavTargetLoc.ToString(),
		CurrentDist2D, Instance.AcceptanceRadius,
		PDStateTreeUtil::PathFollowingResultToString(Result));

	Instance.IssuedGoal = NavTargetLoc;
	Instance.bHasIssuedGoal = true;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPDStateTreeTask_MoveToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat || !Combat->HasValidTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	const APawn* OwnerPawn = Instance.AIController->GetPawn();
	const AActor* Target = Combat->GetCurrentTarget();
	if (!OwnerPawn || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Target 이 움직이면 navmesh 상의 목표도 같이 갱신. 임계값 이상 변동 시 path 재발행.
	const FVector RawTargetLoc = Target->GetActorLocation();
	const FVector NavTargetLoc = PDStateTreeUtil::ProjectToNav(Instance.AIController, RawTargetLoc);
	if (Instance.bHasIssuedGoal)
	{
		const float DriftSq = FVector::DistSquared(NavTargetLoc, Instance.IssuedGoal);
		if (DriftSq > (Instance.ReissueThreshold * Instance.ReissueThreshold))
		{
			const EPathFollowingRequestResult::Type Result =
				Instance.AIController->MoveToLocation(NavTargetLoc, Instance.AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true);
			UE_LOG(LogPDAI, Verbose, TEXT("[MoveToTarget] Reissue: NewLoc=%s, MoveTo=%s"),
				*NavTargetLoc.ToString(), PDStateTreeUtil::PathFollowingResultToString(Result));
			Instance.IssuedGoal = NavTargetLoc;
		}
	}

	// 도달 검사 — 평면(2D) 거리. Target 이 공중에 있어도 그 아래 ground 까지 가면 도달로 처리.
	const float DistSq = FVector::DistSquared2D(OwnerPawn->GetActorLocation(), NavTargetLoc);
	const float ThresholdSq = Instance.AcceptanceRadius * Instance.AcceptanceRadius;

	UE_LOG(LogPDAI, VeryVerbose, TEXT("[MoveToTarget] Tick: DistSq2D=%.1f, ThresholdSq=%.1f"),
		DistSq, ThresholdSq);

	if (DistSq <= ThresholdSq)
	{
		UE_LOG(LogPDAI, Log, TEXT("[MoveToTarget] Succeeded: DistSq2D=%.1f <= ThresholdSq=%.1f"),
			DistSq, ThresholdSq);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FPDStateTreeTask_MoveToTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// Senior: 다음 Task 가 자유롭게 시작하도록 이동 명령을 명시적으로 정지.
	//         Combat Task 가 FocalPoint 만 잡고 사격해야 하는데 잔여 MoveTo 가 살아있으면 회전이 어긋남.
	if (Instance.AIController)
	{
		Instance.AIController->StopMovement();
	}
	Instance.bHasIssuedGoal = false;
}
