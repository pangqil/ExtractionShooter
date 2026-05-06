#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "PDStateTreeTaskCommon.generated.h"

/**
 * Enemy StateTree 의 공용 InstanceData.
 *
 * Senior 관점:
 *  - StateTree Schema 가 StateTreeAIComponent 이므로 ContextActor 는 AIController.
 *    이 USTRUCT 를 모든 Task/Condition 의 InstanceData 로 재사용해 ContextActor 바인딩이 일관되게 동작.
 *  - 추가 파라미터(NewState/Range 등)가 필요한 Task 는 본 구조체를 멤버로 포함하지 않고
 *    Task 별 전용 InstanceData 를 정의하되, AIController 필드 이름을 동일하게 유지.
 *
 * Mid 관점: TObjectPtr 사용 — instance data 가 USTRUCT 라 GC 가 자동 추적.
 */
USTRUCT()
struct PROJECTD_API FPDStateTreeTaskCommonInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;
};

/**
 * 공용 헬퍼 — Pawn 캐스트와 컴포넌트 접근을 한 곳에서 관리.
 * Junior 관점: nullptr 체크가 호출 측에 산재하지 않도록 helper 가 수렴.
 */
namespace PDStateTreeUtil
{
	FORCEINLINE APDBipedEnemy* GetBiped(const AAIController* Controller)
	{
		return Controller ? Cast<APDBipedEnemy>(Controller->GetPawn()) : nullptr;
	}

	FORCEINLINE UPDCombatComponent* GetCombat(const AAIController* Controller)
	{
		const APDBipedEnemy* Biped = GetBiped(Controller);
		return Biped ? Biped->GetCombatComponent() : nullptr;
	}
}
