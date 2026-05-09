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

	const FVector RawTargetLoc = Target->GetActorLocation();
	const FVector NavTargetLoc = PDStateTreeUtil::ProjectToNav(Instance.AIController, RawTargetLoc);

	// 1. Reissue (재발행) 로직
	if (Instance.bHasIssuedGoal)
	{
		const float DriftSq = FVector::DistSquared(NavTargetLoc, Instance.IssuedGoal);
		if (DriftSq > (Instance.ReissueThreshold * Instance.ReissueThreshold))
		{
			const EPathFollowingRequestResult::Type Result =
				Instance.AIController->MoveToLocation(NavTargetLoc, Instance.AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true);

			// [개선] 경로 요청 자체가 실패한 경우 즉시 취소 처리
			if (Result == EPathFollowingRequestResult::Failed)
			{
				UE_LOG(LogPDAI, Warning, TEXT("[MoveToTarget] Reissue Failed. Target unreachable."));
				return EStateTreeRunStatus::Failed;
			}

			Instance.IssuedGoal = NavTargetLoc;
		}
	}

	// 2. 도달 거리 검사
	const float DistSq = FVector::DistSquared2D(OwnerPawn->GetActorLocation(), NavTargetLoc);
	const float ThresholdSq = Instance.AcceptanceRadius * Instance.AcceptanceRadius;

	if (DistSq <= ThresholdSq)
	{
		UE_LOG(LogPDAI, Log, TEXT("[MoveToTarget] Succeeded. DistSq: %.1f"), DistSq);
		return EStateTreeRunStatus::Succeeded;
	}

	// 3. [핵심 개선] PathFollowing 상태 확인 (무한 대기 방지)
	// 거리에 도달하지 못했는데, 이동 컴포넌트가 이동을 멈춘 경우 처리
	const EPathFollowingStatus::Type MoveStatus = Instance.AIController->GetMoveStatus();

	if (MoveStatus == EPathFollowingStatus::Idle)
	{
		// 비동기 경로 탐색 대기(Waiting)를 지나 이동을 완전히 멈췄는데 거리가 멀다면: 
		// 길막힘, NavMesh 이탈, Partial Path 끝 도달 등 이동 불가 상태임
		UE_LOG(LogPDAI, Warning, TEXT("[MoveToTarget] Failed: AI stopped moving but target not reached. DistSq=%.1f"), DistSq);
		return EStateTreeRunStatus::Failed;
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
