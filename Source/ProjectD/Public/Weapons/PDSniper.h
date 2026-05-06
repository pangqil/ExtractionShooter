// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDProjectile.h"
#include "PDSniper.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScopeToggled, bool, bIsScoped);

UCLASS()
class PROJECTD_API APDSniper : public APDWeaponBase
{
	GENERATED_BODY()
	
public:
    APDSniper();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Sniper")
    TSubclassOf<APDProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Sniper")
    TArray<bool> PenetrationPerLevel = { false, false, true };

public:
    virtual void Fire_Implementation() override;
    virtual void Reload_Implementation() override;

private:
    bool CanPenetrate() const;
    void SpawnProjectile(bool bPenetrate);
    FVector GetAimDirection() const;

    UFUNCTION()
    void OnBoltActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
