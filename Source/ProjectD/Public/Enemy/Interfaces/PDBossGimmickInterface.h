#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDBossGimmickInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDBossGimmickInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 보스 전용 기믹 진입점.
 * 현재는 stub. 추후 Phase 전환, GimmickActivated/Deactivated, GimmickPhaseChanged 등
 * 보스 단계 제어 API가 여기로 들어옴.
 *
 * Senior 관점: 보스 기믹은 일반 적과 책임이 다르므로 IPDCombatInterface와 분리하여
 * 일반 적이 불필요한 기믹 API에 노출되지 않도록 함(ISP).
 */
class PROJECTD_API IPDBossGimmickInterface
{
	GENERATED_BODY()

public:
	// 의도적으로 비워둠 — Boss 구현 시점에 함수 추가.
};
