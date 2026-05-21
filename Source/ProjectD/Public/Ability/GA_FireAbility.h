#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "GA_FireAbility.generated.h"

class APDRangedWeaponBase;


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


private:
	FTimerHandle AutoFireHandle;

	UFUNCTION()
	void OnFireReleasedEvent(FGameplayEventData Payload);

	void DoAutoFire();

	APDRangedWeaponBase* GetRangedWeapon() const;
};
