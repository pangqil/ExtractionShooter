#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "PDStateTreeCondition_TargetInRange.generated.h"

class AAIController;

/**
 * Pawn ↔ CombatComponent.CurrentTarget 의 거리 ≤ Range 인지 검사.
 *
 * Mid 관점: DistSquared 비교로 sqrt 회피.
 * Senior 관점: Range 를 인스턴스 데이터로 노출 — 디자이너가 StateTree 에서 AttackRange 와 동일 값을 입력.
 *              C++ 가 CombatComponent.AttackRange 를 직접 읽지 않는 이유:
 *              "Chase 가 도달한 시점" 과 "Combat 이 실제 발사하는 사거리" 를 디자이너가 분리 조정하고 싶을 수 있음.
 *              둘이 동일하면 단순히 같은 값을 입력하면 됨 (히스테리시스 없는 simple 패턴 권장값).
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeCondition_TargetInRangeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float Range = 1500.f;
};

USTRUCT(meta = (DisplayName = "PD Target In Range", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeCondition_TargetInRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPDStateTreeCondition_TargetInRangeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
