#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
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

	/**
	 * MoveToActor/MoveToLocation 결과 enum 을 사람이 읽을 수 있는 문자열로.
	 * EPathFollowingRequestResult 는 namespace::Type 형태라 LexToString 직접 사용이 까다로워 별도 변환.
	 */
	FORCEINLINE const TCHAR* PathFollowingResultToString(EPathFollowingRequestResult::Type InResult)
	{
		switch (InResult)
		{
		case EPathFollowingRequestResult::Failed:            return TEXT("Failed");
		case EPathFollowingRequestResult::AlreadyAtGoal:     return TEXT("AlreadyAtGoal");
		case EPathFollowingRequestResult::RequestSuccessful: return TEXT("RequestSuccessful");
		default:                                             return TEXT("Unknown");
		}
	}

	/**
	 * 임의의 위치를 NavMesh 위 가까운 도달 가능 점으로 projection.
	 *  - 공중/벽 안의 좌표를 받아도 ground 위 navmesh 점으로 보정.
	 *  - 실패 시 원래 위치 그대로 반환.
	 *  - QueryExtent 의 Z=1000 은 공중에서 아래로 떨어뜨려 ground 찾기 위함. 맵 높이에 따라 조정.
	 *
	 * Senior 관점: AI 의 도달 목표는 항상 navmesh 위에 있어야 path 요청이 의미 있음.
	 *              "원본 위치" (예: 공중 NoiseEmitter) 와 "이동 목표" (그 아래 ground) 를 분리.
	 */
	FORCEINLINE FVector ProjectToNav(const UObject* WorldContext, const FVector& Location,
		const FVector& QueryExtent = FVector(500.f, 500.f, 1000.f))
	{
		if (!WorldContext)
		{
			return Location;
		}
		UWorld* World = WorldContext->GetWorld();
		if (!World)
		{
			return Location;
		}

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (!NavSys)
		{
			return Location;
		}

		FNavLocation Projected;
		if (NavSys->ProjectPointToNavigation(Location, Projected, QueryExtent))
		{
			return Projected.Location;
		}
		return Location;
	}
}
