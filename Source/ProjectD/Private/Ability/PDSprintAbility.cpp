#include "Ability/PDSprintAbility.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameplayEffect.h"
#include "GameplayTag/PDGameplayTags.h"

UPDSprintAbility::UPDSprintAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer SprintAbilityTags;
	SprintAbilityTags.AddTag(PDGameplayTags::Input_Sprint);
	SetAssetTags(SprintAbilityTags);

	ActivationOwnedTags.AddTag(PDGameplayTags::State_Sprinting);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Sprinting);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Downed);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_GettingUp);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Dead);
}

void UPDSprintAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const APDCharacterBase* Character = GetPDCharacter();
	const UPDAttributeSet* AttributeSet = GetAttributeSet();
	if (!Character || Character->IsDowned() || Character->IsGettingUp() || Character->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AttributeSet && AttributeSet->GetStamina() < MinStaminaToStart)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SprintEffectHandle = ApplySprintGameplayEffect(SprintEffectClass, Handle, ActorInfo, ActivationInfo);
	SprintCostEffectHandle = ApplySprintGameplayEffect(SprintCostEffectClass, Handle, ActorInfo, ActivationInfo);

	if (!SprintEffectHandle.IsValid() && !SprintCostEffectHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BindStaminaChanged();
}

void UPDSprintAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnbindStaminaChanged();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (SprintEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(SprintEffectHandle);
		}

		if (SprintCostEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(SprintCostEffectHandle);
		}
	}

	SprintEffectHandle.Invalidate();
	SprintCostEffectHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FActiveGameplayEffectHandle UPDSprintAbility::ApplySprintGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass,
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	return ApplyGameplayEffectToOwner(
		Handle,
		ActorInfo,
		ActivationInfo,
		EffectClass.GetDefaultObject(),
		GetAbilityLevel(Handle, ActorInfo));
}

void UPDSprintAbility::BindStaminaChanged()
{
	if (!bEndWhenStaminaEmpty || StaminaChangedDelegateHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		StaminaChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UPDAttributeSet::GetStaminaAttribute()).AddUObject(this, &UPDSprintAbility::OnStaminaChanged);
	}
}

void UPDSprintAbility::UnbindStaminaChanged()
{
	if (!StaminaChangedDelegateHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetStaminaAttribute())
			.Remove(StaminaChangedDelegateHandle);
	}

	StaminaChangedDelegateHandle.Reset();
}

void UPDSprintAbility::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!bEndWhenStaminaEmpty)
	{
		return;
	}

	if (Data.NewValue <= MinStaminaToKeepSprinting)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}
