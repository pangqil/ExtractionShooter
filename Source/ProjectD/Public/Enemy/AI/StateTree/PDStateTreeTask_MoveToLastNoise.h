#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "PDStateTreeTask_MoveToLastNoise.generated.h"

class AAIController;

/**
 * CombatComponent.LastNoiseLocation 으로 이동.
 *  - EnterState : MoveToLocation 발행 (한 번)
 *  - Tick       : (a) 시야 타겟 잡히면 즉시 Failed → 상위가 Combat 로 전이
 *                 (b) Hint 가 만료/소비되면 Failed
 *                 (c) 도달했으면 Hint Clear + Succeeded
 *  - ExitState  : StopMovement
 *
 * Senior 관점:
 *  - "도달=Succeeded, 시야 포착=Failed" 라는 이분법으로 StateTree 그래프가 깔끔해짐.
 *    (Failed 가 곧 "더 좋은 신호가 들어왔다" 의미가 되도록 의도적으로 설계)
 *  - Hint 갱신(같은 상태에서 새 노이즈) 시 IssuedGoal 와 비교해 일정 거리 이상 차이나면 재 MoveTo.
 *
 * Mid 관점:
 *  - MoveToLocation 은 정적 위치 기반이라 자동 repath 가 일어나지 않음. 노이즈 갱신을 따라가려면
 *    수동으로 다시 호출해야 함 → IssuedGoal 캐시로 발행 빈도 제어.
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeTask_MoveToLastNoiseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 청각 위치 도달 판정 거리. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 150.f;

	/** 현재 노이즈 위치가 발행한 goal 에서 이 거리 이상 차이나면 재이동 명령. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float ReissueThreshold = 300.f;

	/** AIController 에 발행한 마지막 goal. (런타임 캐시) */
	UPROPERTY()
	FVector IssuedGoal = FVector::ZeroVector;

	UPROPERTY()
	bool bHasIssuedGoal = false;
};

USTRUCT(meta = (DisplayName = "PD Move To Last Noise", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeTask_MoveToLastNoise : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FPDStateTreeTask_MoveToLastNoise()
	{
		bShouldCallTick = true;
	}

	using FInstanceDataType = FPDStateTreeTask_MoveToLastNoiseInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
