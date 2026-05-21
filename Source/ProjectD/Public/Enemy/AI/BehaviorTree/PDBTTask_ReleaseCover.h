#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_ReleaseCover.generated.h"

UCLASS(meta = (DisplayName = "PD Release Cover"))
class PROJECTD_API UPDBTTask_ReleaseCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_ReleaseCover();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
