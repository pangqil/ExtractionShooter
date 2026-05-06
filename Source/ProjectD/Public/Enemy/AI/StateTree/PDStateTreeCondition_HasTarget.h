#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "PDStateTreeCondition_HasTarget.generated.h"

class AAIController;

/**
 * CombatComponent.HasValidTarget() 그대로 노출.
 * Senior 관점: StateTree 디자이너가 "타겟 있음/없음" 만으로 분기할 수 있게 하는 단일 진실 원천.
 *              PerceptionComponent 가 아닌 CombatComponent 를 보는 이유 — perception 자극은
 *              순간적이지만 CombatComponent 의 CurrentTarget 은 실제 추적 대상이라 의미가 다름.
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeCondition_HasTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(meta = (DisplayName = "PD Has Target", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeCondition_HasTarget : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPDStateTreeCondition_HasTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
