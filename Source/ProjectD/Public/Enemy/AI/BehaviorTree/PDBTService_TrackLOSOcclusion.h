#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_TrackLOSOcclusion.generated.h"

/**
 * LOS 차단 누적 시간 + 수류탄 쿨다운 추적.
 *  - HasLOSToTarget(=true) 시 TimeSinceLastLOS=0, LastSeenLocation 을 타겟 현재 위치로 갱신.
 *  - HasLOSToTarget(=false) 시 TimeSinceLastLOS 누적.
 *  - TimeSinceLastLOS >= OcclusionThreshold && LastGrenadeTime 쿨다운 경과 → bCanThrowGrenade=true.
 *
 * 선행: 동일 노드에 UPDBTService_UpdateCombatBB 가 먼저 붙어 HasLOSToTarget 키를 갱신해야 함.
 */
UCLASS(meta = (DisplayName = "PD Track LOS Occlusion"))
class PROJECTD_API UPDBTService_TrackLOSOcclusion : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_TrackLOSOcclusion();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** LOS 차단 누적이 이 시간(초)을 넘으면 grenade 후보. */
	UPROPERTY(EditAnywhere, Category = "PD|Grenade", meta = (ClampMin = "0.0"))
	float OcclusionThreshold = 2.5f;

	/** 마지막 투척 이후 다음 투척까지 최소 간격(초). */
	UPROPERTY(EditAnywhere, Category = "PD|Grenade", meta = (ClampMin = "0.0"))
	float GrenadeCooldown = 12.f;
};
