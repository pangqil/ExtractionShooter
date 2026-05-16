#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "PDShotgun.generated.h"


UCLASS(Blueprintable)
class PROJECTD_API APDShotgun : public APDRangedWeaponBase
{
	GENERATED_BODY()

public:
    APDShotgun();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun")
    TArray<int32> PelletCountPerLevel = { 5, 7, 9 };

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun")
    float SpreadAngle = 15.f;

public:
    virtual void Fire_Implementation() override;
    virtual void Reload_Implementation() override;

private:
    int32 GetCurrentPelletCount() const;
    void PerformPelletTraces(TArray<FHitResult>& OutHits);
};
