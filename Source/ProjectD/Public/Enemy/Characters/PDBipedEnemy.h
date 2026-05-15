#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDEnemyBase.h"
#include "PDBipedEnemy.generated.h"

class UPDCombatComponent;

/**
 * 이족 보행 적 추상 베이스.
 *  - Stamina 는 PDCharacterBase 의 GAS UPDAttributeSet (Stamina/MaxStamina) 을 단일 진실 원천으로 사용.
 *  - GetStaminaStatus 를 AttributeSet 퍼센트 + 본 클래스 임계값으로 분류.
 *  - CombatComponent 의무 보유.
 *
 * "Biped" 카테고리를 클래스 계층으로 명시 → 4족/비행 적이 추가될 때
 * Stamina/이족 가정이 새지 않도록 격리.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDBipedEnemy : public APDEnemyBase
{
	GENERATED_BODY()

public:
	APDBipedEnemy();

	virtual EPDStaminaStatus GetStaminaStatus_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	FORCEINLINE UPDCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	/** Stamina 퍼센트 임계값 — 이상이면 Full. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaFullThreshold = 0.8f;

	/** Stamina 퍼센트 임계값 — 이상이면 Optimal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaOptimalThreshold = 0.5f;

	/** Stamina 퍼센트 임계값 — 이상이면 Low, 미만이면 Exhausted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Stamina",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaLowThreshold = 0.2f;

private:
	UPROPERTY(VisibleAnywhere, Category = "PD|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDCombatComponent> CombatComponent;
};
