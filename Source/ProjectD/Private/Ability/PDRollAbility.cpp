#include "Ability/PDRollAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Animation/AnimInstance.h"

UPDRollAbility::UPDRollAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 구르기 중 재발동 방지
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
}

void UPDRollAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDCharacterBase* Character = GetPDCharacter();
	if (!Character || !RollMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 구르기 방향 계산 후 캐릭터 즉시 회전
	const FVector RollDir = GetRollDirection();
	if (!RollDir.IsNearlyZero())
		Character->SetActorRotation(RollDir.Rotation());

	// 2. State_Rolling 태그 부여
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		ASC->AddLooseGameplayTag(PDGameplayTags::State_Rolling);

	// 3. 이동 임펄스
	Character->LaunchCharacter(RollDir * RollImpulseStrength, true, false);

	// 4. 몽타주 재생
	PlayAbilityMontage(RollMontage);

	// 5. 몽타주 종료 바인딩
	if (USkeletalMeshComponent* SkelMesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInst = SkelMesh->GetAnimInstance())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UPDRollAbility::OnRollMontageEnded);
			AnimInst->Montage_SetEndDelegate(EndDelegate, RollMontage);
		}
	}
}

void UPDRollAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// State_Rolling 태그 제거
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		ASC->RemoveLooseGameplayTag(PDGameplayTags::State_Rolling);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPDRollAbility::OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

FVector UPDRollAbility::GetRollDirection() const
{
	APDCharacterBase* Character = GetPDCharacter();
	if (!Character) return FVector::ForwardVector;

	if (bUseInputDirection)
	{
		const FVector Input = Character->GetCharacterMovement()->GetLastInputVector();
		if (!Input.IsNearlyZero())
			return Input.GetSafeNormal2D();
	}

	return Character->GetActorForwardVector();
}
