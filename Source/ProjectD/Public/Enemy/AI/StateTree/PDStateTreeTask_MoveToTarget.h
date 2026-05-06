#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "PDStateTreeTask_MoveToTarget.generated.h"

class AAIController;

/**
 * CombatComponent 의 CurrentTarget 까지 이동.
 *  - EnterState : MoveToActor 발행 (AcceptanceRadius)
 *  - Tick       : 타겟 유효성 + 도달 거리 체크
 *  - ExitState  : 이동 정지 (다음 Task 가 깔끔하게 시작하도록)
 *
 * Senior 관점: MoveToActor 는 goal actor 가 움직이면 자동 repath — 별도 timer 불필요.
 *              StateTree 가 분기 조건으로 Distance 를 보고 외부에서 전이시킬 수도 있지만,
 *              이 Task 는 "도달했다" 라는 명시적 Succeeded 신호도 함께 제공해 그래프 표현력을 높임.
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeTask_MoveToTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 타겟 도달 판정 거리. 보통 AttackRange 와 동일하게 디자이너가 입력. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 1500.f;
};

USTRUCT(meta = (DisplayName = "PD Move To Target", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeTask_MoveToTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FPDStateTreeTask_MoveToTarget()
	{
		// 거리 체크와 타겟 유효성을 매 틱 검사 — Combat 진입 판정의 응답성 위해.
		bShouldCallTick = true;
	}

	using FInstanceDataType = FPDStateTreeTask_MoveToTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
