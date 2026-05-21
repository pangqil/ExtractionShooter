#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_SetCoverPeek.generated.h"

UCLASS(meta = (DisplayName = "PD Set Cover Peek"))
class PROJECTD_API UPDBTTask_SetCoverPeek : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_SetCoverPeek();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Cover")
	bool bPeek = true;
};
