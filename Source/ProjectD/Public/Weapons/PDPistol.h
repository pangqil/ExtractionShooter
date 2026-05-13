#pragma once

#include "CoreMinimal.h"
#include "Weapons/PDWeaponBase.h"
#include "PDPistol.generated.h"

// 반자동 권총: 클릭마다 한 발, FireRate를 쿨다운으로 사용.
UCLASS(Blueprintable)
class PROJECTD_API APDPistol : public APDWeaponBase
{
	GENERATED_BODY()

public:
	APDPistol();

	virtual void Fire_Implementation() override;
	virtual void Reload_Implementation() override;

private:
	bool PerformLineTrace(FHitResult& OutHit);
};
