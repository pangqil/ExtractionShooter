#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "PDBTDecorator_TargetInRange.generated.h"

/**
 * BB.TargetActor 와 Pawn 의 거리 ≤ Range 일 때 true.
 * Range 는 직접 입력하거나 BB Float 키(예: AttackRange) 로 동적으로 사용 가능.
 */
UCLASS(meta = (DisplayName = "PD Target In Range"))
class PROJECTD_API UPDBTDecorator_TargetInRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UPDBTDecorator_TargetInRange();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Range")
	struct FBlackboardKeySelector TargetActorKey;

	/** 직접 지정한 Range. UseBlackboardKey=false 일 때만 사용. */
	UPROPERTY(EditAnywhere, Category = "PD|Range", meta = (ClampMin = "0.0", EditCondition = "!bUseBlackboardKey"))
	float Range = 1500.f;

	UPROPERTY(EditAnywhere, Category = "PD|Range")
	bool bUseBlackboardKey = false;

	UPROPERTY(EditAnywhere, Category = "PD|Range", meta = (EditCondition = "bUseBlackboardKey"))
	struct FBlackboardKeySelector RangeKey;
};
