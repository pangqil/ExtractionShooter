#include "Ability/PDReviveAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTag/PDGameplayTags.h"

UPDReviveAbility::UPDReviveAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationOwnedTags.AddTag(PDGameplayTags::State_Reviving);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = PDGameplayTags::Event_Revive_Start;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UPDReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDCharacterBase* Reviver = GetPDCharacter();
	ReviveTarget = TriggerEventData
		? Cast<APDCharacterBase>(const_cast<AActor*>(TriggerEventData->Target.Get()))
		: nullptr;
	bReceivedCancelEvent = false;

	if (!Reviver || !ReviveTarget || Reviver == ReviveTarget || Reviver->IsDowned() || Reviver->IsGettingUp() || Reviver->IsDead() || !ReviveTarget->IsDowned())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ReviveTarget->BeginReviveInteraction(Reviver))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayReviveLoopMontage();

	UAbilityTask_WaitGameplayEvent* CancelEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, PDGameplayTags::Event_Revive_Cancel);
	CancelEventTask->EventReceived.AddDynamic(this, &UPDReviveAbility::OnReviveCancelEvent);
	CancelEventTask->ReadyForActivation();

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ReviveTarget->GetReviveTime());
	DelayTask->OnFinish.AddDynamic(this, &UPDReviveAbility::OnReviveDelayFinished);
	DelayTask->ReadyForActivation();
}

void UPDReviveAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bWasCancelled && !bReceivedCancelEvent && ReviveTarget)
	{
		if (APDCharacterBase* Reviver = GetPDCharacter())
		{
			ReviveTarget->CancelReviveInteraction(Reviver);
		}
	}

	StopReviveMontages(bWasCancelled ? 0.1f : 0.15f);

	ReviveTarget = nullptr;
	bReceivedCancelEvent = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPDReviveAbility::OnReviveDelayFinished()
{
	StopReviveMontages(0.1f);

	const float CompleteDuration = PlayReviveCompleteMontage();
	if (CompleteDuration > KINDA_SMALL_NUMBER)
	{
		UAbilityTask_WaitDelay* CompleteDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, CompleteDuration);
		CompleteDelayTask->OnFinish.AddDynamic(this, &UPDReviveAbility::OnReviveCompleteDelayFinished);
		CompleteDelayTask->ReadyForActivation();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPDReviveAbility::OnReviveCompleteDelayFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPDReviveAbility::OnReviveCancelEvent(FGameplayEventData Payload)
{
	if (ReviveTarget && Payload.Target.Get() != ReviveTarget)
	{
		return;
	}

	bReceivedCancelEvent = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

UAnimInstance* UPDReviveAbility::GetReviverAnimInstance() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->SkeletalMeshComponent.IsValid())
	{
		return nullptr;
	}

	return ActorInfo->SkeletalMeshComponent->GetAnimInstance();
}

void UPDReviveAbility::PlayReviveLoopMontage()
{
	UAnimInstance* AnimInstance = GetReviverAnimInstance();
	if (!AnimInstance || !ReviveLoopMontage)
	{
		return;
	}

	AnimInstance->Montage_Play(ReviveLoopMontage, MontagePlayRate);
}

float UPDReviveAbility::PlayReviveCompleteMontage()
{
	UAnimInstance* AnimInstance = GetReviverAnimInstance();
	if (!AnimInstance || !ReviveCompleteMontage)
	{
		return 0.f;
	}

	AnimInstance->Montage_Play(ReviveCompleteMontage, MontagePlayRate);
	if (ReviveCompleteDuration > KINDA_SMALL_NUMBER)
	{
		return ReviveCompleteDuration;
	}

	return MontagePlayRate > KINDA_SMALL_NUMBER
		? ReviveCompleteMontage->GetPlayLength() / MontagePlayRate
		: ReviveCompleteMontage->GetPlayLength();
}

void UPDReviveAbility::StopReviveMontages(float BlendOutTime)
{
	if (UAnimInstance* AnimInstance = GetReviverAnimInstance())
	{
		if (ReviveLoopMontage && AnimInstance->Montage_IsPlaying(ReviveLoopMontage))
		{
			AnimInstance->Montage_Stop(BlendOutTime, ReviveLoopMontage);
		}

		if (ReviveCompleteMontage && AnimInstance->Montage_IsPlaying(ReviveCompleteMontage))
		{
			AnimInstance->Montage_Stop(BlendOutTime, ReviveCompleteMontage);
		}
	}
}
