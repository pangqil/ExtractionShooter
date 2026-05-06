#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDStateTreeTask_SetEnemyState.generated.h"

class AAIController;

/**
 * 진입 시 Pawn(APDEnemyBase)->SetEnemyState(NewState) 를 호출하고 Running 으로 머무름.
 * Senior 관점: 상태 enum 동기화 전용 instant task — 별도 진행 로직이 없으므로 Tick 미사용.
 *              Running 을 반환해 부모 State 가 살아있는 동안 OnEnemyStateChanged 가 한 번만 발사되도록 함.
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeTask_SetEnemyStateInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EPDEnemyState NewState = EPDEnemyState::Idle;
};

USTRUCT(meta = (DisplayName = "PD Set Enemy State", Category = "PD|AI"))
struct PROJECTD_API FPDStateTreeTask_SetEnemyState : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPDStateTreeTask_SetEnemyStateInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
