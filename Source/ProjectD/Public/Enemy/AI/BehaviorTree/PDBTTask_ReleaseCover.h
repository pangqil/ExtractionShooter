#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PDBTTask_ReleaseCover.generated.h"

/**
 * 현재 점유 중인 cover 해제 + BB.CoverActor / CoverLocation / bHasCoverTarget 클리어.
 * APDEliteSoldier.SetInCover(nullptr) 호출로 피크 상태도 함께 종료.
 */
UCLASS(meta = (DisplayName = "PD Release Cover"))
class PROJECTD_API UPDBTTask_ReleaseCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPDBTTask_ReleaseCover();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
