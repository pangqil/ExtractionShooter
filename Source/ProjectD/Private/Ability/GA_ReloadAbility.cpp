#include "Ability/GA_ReloadAbility.h"

#include "Characters/PDPlayerCharacter.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

UGA_ReloadAbility::UGA_ReloadAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_ReloadAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APDPlayerCharacter* Char = Cast<APDPlayerCharacter>(GetPDCharacter());
	APDRangedWeaponBase* Weapon = Char ? Cast<APDRangedWeaponBase>(Char->GetCurrentWeapon()) : nullptr;

	if (!Weapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WeaponPtr = Weapon;

	// 장전 완료 델리게이트 구독
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

		// 발사 등 외부 원인으로 취소된 경우 리로드 상태 강제 초기화
		if (bWasCancelled)
			WeaponPtr->CancelReload();

		WeaponPtr.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ReloadAbility::OnWeaponReloaded(APDWeaponBase* Weapon)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
