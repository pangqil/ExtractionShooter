#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDEnemyBase.h"
#include "PDBipedEnemy.generated.h"

class UPDStatComponent;
class UPDCombatComponent;

/**
 * 이족 보행 적의 추상 베이스.
 *  - UPDStatComponent + UPDCombatComponent 를 의무적으로 보유.
 *  - bUseBattery=true 로 Battery(스태미너) 시스템 활성.
 *  - GetBatteryStatus 를 StatComponent의 실제 값으로 오버라이드.
 *
 * Senior 관점: "Biped" 라는 추상 카테고리를 클래스 계층으로 명시함으로써,
 *              4족(Quadruped) / 비행(Flying) 등 다른 형태의 적이 추가될 때
 *              본 클래스를 상속하지 않아 Battery/이족 가정이 새는 것을 방지.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDBipedEnemy : public APDEnemyBase
{
	GENERATED_BODY()

public:
	APDBipedEnemy();

	//~ Begin IPDCombatInterface
	virtual EPDBatteryStatus GetBatteryStatus_Implementation() const override;
	//~ End IPDCombatInterface

	UFUNCTION(BlueprintPure, Category = "PD|Stat")
	FORCEINLINE UPDStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	FORCEINLINE UPDCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDCombatComponent> CombatComponent;
};
