#include "Ability/PDCoverAimAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"

UPDCoverAimAbility::UPDCoverAimAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ActivationRequiredTags.AddTag(PDGameplayTags::Cover_Active);
}

void UPDCoverAimAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    APDCharacterBase* Char = GetPDCharacter();
    if (!ASC || !Char)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ASC->RemoveLooseGameplayTag(PDGameplayTags::Cover_Active);
    ASC->AddLooseGameplayTag(PDGameplayTags::State_CoverAim);

    Char->GetCharacterMovement()->SetDefaultMovementMode();

    // 현재 캐릭터가 벽을 향하고 있으므로 180도 반전 = 적을 향함
    const FVector AimDir = -Char->GetActorForwardVector();
    const FVector AimPos = Char->GetActorLocation() + AimDir * PeekOffset;
    const FRotator AimRot = AimDir.Rotation();

    BP_OnStartAim(AimPos, AimRot);
}

void UPDCoverAimAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    APDCharacterBase* Char = GetPDCharacter();

    if (ASC)
    {
        ASC->RemoveLooseGameplayTag(PDGameplayTags::State_CoverAim);
        ASC->AddLooseGameplayTag(PDGameplayTags::Cover_Active);
    }
    if (Char)
    {
        Char->GetCharacterMovement()->DisableMovement();
        const FVector CoverDir = -Char->GetActorForwardVector();
        BP_OnReturnToCover(Char->GetActorLocation(), CoverDir.Rotation());
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
