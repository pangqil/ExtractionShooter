#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "GA_FireAbility.generated.h"

class APDRangedWeaponBase;

/**
 * 원거리 무기 발사 어빌리티 베이스.
 *
 * BP 서브클래스에서 설정:
 *   - ActivationRequiredTags : Weapon.Type.Rifle / Shotgun / Sniper 중 하나
 *   - bAutoFire              : Rifle은 true, 나머지는 false
 *
 * 자동화기 루프: Weapon->Fire() → FireRate 타이머 반복 → Input.FireReleased 이벤트 수신 시 종료.
 * 리로드 중단:   CancelAbilitiesWithTag 에 Weapon.State.Reloading 추가하면 GA_Reload 자동 취소.
 */
UCLASS(Blueprintable)
class PROJECTD_API UGA_FireAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_FireAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	// 연사 여부는 무기의 bFullAuto 에서 자동으로 읽음 (별도 설정 불필요)

private:
	FTimerHandle AutoFireHandle;

	UFUNCTION()
	void OnFireReleasedEvent(FGameplayEventData Payload);

	void DoAutoFire();

	APDRangedWeaponBase* GetRangedWeapon() const;
};
