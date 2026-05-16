#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "GA_ReloadAbility.generated.h"

class APDRangedWeaponBase;
class APDWeaponBase;

/**
 * 원거리 무기 장전 어빌리티.
 *
 * - 활성화 중 ASC에 Weapon.State.Reloading 태그 보유
 *   → GA_FireAbility의 CancelAbilitiesWithTag가 이 태그를 참조해 발사 시 자동 취소됨.
 * - 장전 완료(OnWeaponReloaded) 또는 취소(bWasCancelled) 시 종료.
 * - ActivationRequiredTags는 BP에서 설정 (예: Weapon.Type.Rifle | Shotgun | Sniper).
 *   또는 빈 채로 두고 C++ 에서 RangedWeapon Cast 로 걸러냄.
 */
UCLASS(Blueprintable)
class PROJECTD_API UGA_ReloadAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_ReloadAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	TWeakObjectPtr<APDRangedWeaponBase> WeaponPtr;

	UFUNCTION()
	void OnWeaponReloaded(APDWeaponBase* Weapon);
};
