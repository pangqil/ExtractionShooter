#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "PDStateTreeCondition_HasNoiseHint.generated.h"

class AAIController;

/**
 * CombatComponent.HasNoiseHint() 을 그대로 노출.
 * Senior 관점: HasValidTarget(시각) 과 의도적으로 다른 Condition — 디자이너가
 *              "보임" 과 "들림" 을 분리해 분기 우선순위를 잡을 수 있도록.
 *              (시각 우선 → Combat / 청각만 → Investigate / 둘 다 없음 → Idle)
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeCondition_HasNoiseHintInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(meta = (DisplayName = "PD Has Noise Hint", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeCondition_HasNoiseHint : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPDStateTreeCondition_HasNoiseHintInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
