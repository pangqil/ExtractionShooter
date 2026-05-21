#include "Ability/GA_ReloadAbility.h"

#include "Characters/PDPlayerCharacter.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

UGA_ReloadAbility::UGA_ReloadAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_ReloadAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDPlayerCharacter* Char = Cast<APDPlayerCharacter>(GetPDCharacter());
	APDRangedWeaponBase* Weapon = Char ? Cast<APDRangedWeaponBase>(Char->GetCurrentWeapon()) : nullptr;

	if (!Weapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!Weapon->CanReload())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WeaponPtr = Weapon;
	Weapon->OnWeaponReloaded.RemoveDynamic(this, &UGA_ReloadAbility::OnWeaponReloaded);
	Weapon->OnWeaponReloaded.AddDynamic(this, &UGA_ReloadAbility::OnWeaponReloaded);

	Weapon->Reload();
}

void UGA_ReloadAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WeaponPtr.IsValid())
	{
		WeaponPtr->OnWeaponReloaded.RemoveDynamic(this, &UGA_ReloadAbility::OnWeaponReloaded);

		if (bWasCancelled)
		{
			WeaponPtr->CancelReload();
		}

		WeaponPtr.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ReloadAbility::OnWeaponReloaded(APDWeaponBase* Weapon)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
