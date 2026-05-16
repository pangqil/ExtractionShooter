#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "PDMeleeWeaponBase.generated.h"

UCLASS(Blueprintable)
class PROJECTD_API APDMeleeWeaponBase : public APDWeaponBase
{
	GENERATED_BODY()

public:
	APDMeleeWeaponBase();

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Melee")
	FORCEINLINE float GetSweepRadius() const { return SweepRadius; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Melee")
	FORCEINLINE float GetSweepRange() const { return SweepRange; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Melee")
	FORCEINLINE FName GetHitSocketName() const { return HitSocketName; }

protected:
	UPROPERTY(EditDefaultsOnly, Category="PD|Weapon|Melee")
	float SweepRadius = 30.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Weapon|Melee")
	float SweepRange = 80.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Weapon|Melee")
	FName HitSocketName = TEXT("weapon_r");
};
