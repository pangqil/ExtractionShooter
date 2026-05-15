#include "Ability/PDMeleeAttackAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Characters/PDPlayerCharacter.h"
#include "Weapons/PDWeaponBase.h"
#include "Interfaces/PDDamageable.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UPDMeleeAttackAbility::UPDMeleeAttackAbility()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationRequiredTags.AddTag(PDGameplayTags::Weapon_Type_Melee);
	AbilityTags.AddTag(PDGameplayTags::State_MeleeAttacking);
	BlockAbilitiesWithTag.AddTag(PDGameplayTags::State_MeleeAttacking);
}

void UPDMeleeAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (!AttackMontage || AttackSections.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	HitActors.Reset();

	const FName Section=AttackSections[FMath::RandRange(0, AttackSections.Num()-1)];

	UAbilityTask_PlayMontageAndWait* MontageTask=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, 1.f, Section);
	MontageTask->OnCompleted.AddDynamic(this, &UPDMeleeAttackAbility::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UPDMeleeAttackAbility::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UPDMeleeAttackAbility::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UPDMeleeAttackAbility::OnMontageFinished);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* EventTask=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, PDGameplayTags::Anim_Notify_MeleeHit);
	EventTask->EventReceived.AddDynamic(this, &UPDMeleeAttackAbility::OnMeleeHitReceived);
	EventTask->ReadyForActivation();
}

void UPDMeleeAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	HitActors.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPDMeleeAttackAbility::OnMeleeHitReceived(FGameplayEventData Payload)
{
	PerformSweep();
}

void UPDMeleeAttackAbility::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPDMeleeAttackAbility::PerformSweep()
{
	APDPlayerCharacter* Char=Cast<APDPlayerCharacter>(GetPDCharacter());
	if (!Char) return;

	APDWeaponBase* Weapon=Char->GetCurrentWeapon();
	if (!Weapon) return;

	const float Damage=Weapon->GetCurrentStats().Damage;
	const FVector Start=Char->GetMesh()->GetSocketLocation(WeaponSocketName);
	const FVector End=Start+Char->GetActorForwardVector()*SweepRange;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Char);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(SweepRadius), Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor=Hit.GetActor();
		if (!IsValid(HitActor) || HitActors.Contains(HitActor)) continue;
		if (!HitActor->Implements<UPDDamageable>()) continue;

		HitActors.Add(HitActor);

		FPDDamageInfo DamageInfo;
		DamageInfo.BaseDamage=Damage;
		DamageInfo.Instigator=Char;
		DamageInfo.HitResult=Hit;
		IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
	}
}
