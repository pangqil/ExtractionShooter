#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "GA_ReloadAbility.generated.h"

class APDRangedWeaponBase;
class APDWeaponBase;


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
