#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "PDStateTreeTask_FireAtTarget.generated.h"

class AAIController;

/**
 * 매 Tick 타겟을 바라보고 RequestAttack 시도.
 *  - 실제 발사 모션/이펙트/데미지는 UPDCombatComponent::OnAttackRequested 를 구독한
 *    BP_Soldier 또는 무기 컴포넌트가 처리 (SRP).
 *  - 쿨다운/사거리 검증은 CombatComponent 내부.
 *
 * Senior 관점: 본 Task 는 "지속적 의도(공격하라)" 만 표현 — Tick 이 부담스러운 환경이라면
 *              Cooldown 만큼 sleep 하도록 Timer 로 바꿀 수 있으나, 1초 단위 RequestAttack 호출은
 *              사실상 무비용(쿨다운에서 즉시 return)이라 단순 Tick 유지.
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeTask_FireAtTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(meta = (DisplayName = "PD Fire At Target", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeTask_FireAtTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FPDStateTreeTask_FireAtTarget()
	{
		bShouldCallTick = true;
	}

	using FInstanceDataType = FPDStateTreeTask_FireAtTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
