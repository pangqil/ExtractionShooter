#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_ThrowGrenade.generated.h"

/**
 * BB.LastSeenLocation 으로 APDEliteSoldier.ThrowGrenadeAt 호출.
 *  - 실제 수류탄 스폰/궤적은 BP 측 ThrowGrenadeAt override 에서 처리(본 코드는 훅만 호출).
 *  - 호출 직후 LastGrenadeTime=World.Now, bCanThrowGrenade=false 로 마킹 → 쿨다운 진입.
 *  - PostThrowDelay 동안 InProgress 유지(애니메이션 시간 확보). 0 이면 즉시 Succeeded.
 */
UCLASS(meta = (DisplayName = "PD Throw Grenade"))
class PROJECTD_API UPDBTTask_ThrowGrenade : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_ThrowGrenade();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 던지기 애니메이션/연출 시간(초). 0 이면 즉시 종료. */
	UPROPERTY(EditAnywhere, Category = "PD|Grenade", meta = (ClampMin = "0.0"))
	float PostThrowDelay = 1.2f;

private:
	struct FMemory
	{
		float RemainingTime = 0.f;
	};
};
