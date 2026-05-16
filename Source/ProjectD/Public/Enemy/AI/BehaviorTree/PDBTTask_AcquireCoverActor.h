#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_AcquireCoverActor.generated.h"

/**
 * 스톡 RunEQSQuery 가 BB.CoverActor 에 APDCoverBase 후보를 써준 직후 호출.
 *  - TryOccupy 성공 시 APDEliteSoldier.SetInCover(actor) + BB.CoverLocation 에 SnapLocation 기록.
 *  - 실패(이미 점유/파괴) 시 Failed → Selector 의 다음 분기(예: 동적 cover 폴백) 평가.
 *
 * BlackboardKey 기본값 = CoverActor. 디자이너가 다른 키 쓰고 싶으면 BT 자산에서 교체.
 */
UCLASS(meta = (DisplayName = "PD Acquire Cover Actor"))
class PROJECTD_API UPDBTTask_AcquireCoverActor : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_AcquireCoverActor();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
