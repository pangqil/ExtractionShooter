// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Type/Types.h"
#include "PDRifle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireModeChanged, EFireMode, NewFireMode);

UCLASS(Blueprintable)
class PROJECTD_API APDRifle : public APDRangedWeaponBase
{
	GENERATED_BODY()

public:
    APDRifle();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Rifle")
    EFireMode FireMode = EFireMode::Auto;

public:
    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnFireModeChanged OnFireModeChanged;

    virtual void Fire_Implementation() override;
    virtual void Reload_Implementation() override;

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Rifle")
    void ToggleFireMode();

    UFUNCTION(BlueprintPure, Category = "PD|Weapon|Rifle")
    FORCEINLINE EFireMode GetFireMode() const { return FireMode; }

private:
    bool PerformLineTrace(FHitResult& OutHit);
};