#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_AcquireCoverActor.generated.h"

UCLASS(meta = (DisplayName = "PD Acquire Cover Actor"))
class PROJECTD_API UPDBTTask_AcquireCoverActor : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_AcquireCoverActor();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
