#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDCombatInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 전투 시스템(BT/Decorator/CombatComponent)이 액터의 팀/생사/스태미너를
 * 구체 클래스 의존 없이 조회하기 위한 경량 인터페이스.
 */
class PROJECTD_API IPDCombatInterface
{
	GENERATED_BODY()

public:
	/** 0=Neutral, 1=Player, 2=Hostile, ... */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Combat")
	uint8 GetTeamID() const;

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Combat")
	bool IsAlive() const;

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Combat")
	EPDStaminaStatus GetStaminaStatus() const;
};
