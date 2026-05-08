#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "PDBTDecorator_HasLOS.generated.h"

/**
 * Pawn → BB.TargetActor 직선 시야 체크 (Visibility 채널).
 * AIPerception 의 Sight 는 Lose 까지 시간이 걸리므로, 즉발적인 LOS 가드가 필요할 때 사용.
 */
UCLASS(meta = (DisplayName = "PD Has LOS"))
class PROJECTD_API UPDBTDecorator_HasLOS : public UBTDecorator
{
	GENERATED_BODY()

public:
	UPDBTDecorator_HasLOS();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|LOS")
	struct FBlackboardKeySelector TargetActorKey;
};
