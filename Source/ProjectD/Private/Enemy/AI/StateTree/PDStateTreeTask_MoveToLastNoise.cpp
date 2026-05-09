#include "Enemy/AI/StateTree/PDStateTreeTask_MoveToLastNoise.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FPDStateTreeTask_MoveToLastNoise::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat || !Combat->HasNoiseHint() || !Instance.AIController)
	{
		// 디버그: Task 진입은 했지만 즉시 Failed — 어떤 전제가 빠졌는지 명시.
		UE_LOG(LogPDAI, Verbose,
			TEXT("[MoveToLastNoise] EnterState Failed: AIController=%s, Combat=%s, HasNoiseHint=%s"),
			Instance.AIController ? TEXT("OK") : TEXT("null"),
			Combat ? TEXT("OK") : TEXT("null"),
			(Combat && Combat->HasNoiseHint()) ? TEXT("true") : TEXT("false"));
		return EStateTreeRunStatus::Failed;
	}

	const FVector RawNoiseLoc = Combat->GetLastNoiseLocation();
	// NoiseEmitter 가 공중/벽 안에 있어도 navmesh 위 가까운 점으로 보정 — 도달 가능한 목표로 변환.
	const FVector NoiseLoc = PDStateTreeUtil::ProjectToNav(Instance.AIController, RawNoiseLoc);

	const EPathFollowingRequestResult::Type Result =
		Instance.AIController->MoveToLocation(NoiseLoc, Instance.AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true);

	// 디버그: 진입 체인 4단계 — MoveTo 발행 결과. Failed 면 navmesh projection 도 실패한 도달 불가 위치.
	UE_LOG(LogPDAI, Log, TEXT("[MoveToLastNoise] EnterState: RawLoc=%s, NavLoc=%s, AcceptanceRadius=%.1f, MoveTo=%s"),
		*RawNoiseLoc.ToString(), *NoiseLoc.ToString(),
		Instance.AcceptanceRadius, PDStateTreeUtil::PathFollowingResultToString(Result));

	Instance.IssuedGoal = NoiseLoc;
	Instance.bHasIssuedGoal = true;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPDStateTreeTask_MoveToLastNoise::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat || !Instance.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	// (a) 시각 타겟 잡힘 → 청각 hint 무효, 상위가 Combat 로 전이하도록 Failed.
	if (Combat->HasValidTarget())
	{
		Combat->ClearNoiseHint();
		return EStateTreeRunStatus::Failed;
	}

	// (b) Hint 가 만료/외부에서 Clear 됨.
	if (!Combat->HasNoiseHint())
	{
		return EStateTreeRunStatus::Failed;
	}

	const APawn* Pawn = Instance.AIController->GetPawn();
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 노이즈 갱신 추적 — 임계값 초과 시 재이동 명령. 매 평가 시 navmesh projection.
	const FVector RawCurrentNoiseLoc = Combat->GetLastNoiseLocation();
	const FVector CurrentNoiseLoc = PDStateTreeUtil::ProjectToNav(Instance.AIController, RawCurrentNoiseLoc);
	if (Instance.bHasIssuedGoal)
	{
		const float DriftSq = FVector::DistSquared(CurrentNoiseLoc, Instance.IssuedGoal);
		if (DriftSq > (Instance.ReissueThreshold * Instance.ReissueThreshold))
		{
			const EPathFollowingRequestResult::Type Result =
				Instance.AIController->MoveToLocation(CurrentNoiseLoc, Instance.AcceptanceRadius, /*bStopOnOverlap=*/false, /*bUsePathfinding=*/true);
			UE_LOG(LogPDAI, Verbose, TEXT("[MoveToLastNoise] Reissue MoveTo: NewLoc=%s, MoveTo=%s"),
				*CurrentNoiseLoc.ToString(), PDStateTreeUtil::PathFollowingResultToString(Result));
			Instance.IssuedGoal = CurrentNoiseLoc;
		}
	}

	// (c) 도달 검사 — 평면(2D) 거리. NoiseEmitter 가 공중에 있더라도 그 아래 ground 까지 가면
	//     도달로 처리. 3D 거리는 Z 차이로 영원히 못 닿는 케이스를 만듦.
	const float DistSq = FVector::DistSquared2D(Pawn->GetActorLocation(), CurrentNoiseLoc);
	const float ThresholdSq = Instance.AcceptanceRadius * Instance.AcceptanceRadius;
	if (DistSq <= ThresholdSq)
	{
		UE_LOG(LogPDAI, Log, TEXT("[MoveToLastNoise] Succeeded: DistSq2D=%.1f <= ThresholdSq=%.1f"),
			DistSq, ThresholdSq);
		Combat->ClearNoiseHint();
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FPDStateTreeTask_MoveToLastNoise::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (Instance.AIController)
	{
		Instance.AIController->StopMovement();
	}
	Instance.bHasIssuedGoal = false;
}
