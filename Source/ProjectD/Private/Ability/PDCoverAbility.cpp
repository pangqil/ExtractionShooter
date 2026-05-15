#include "Ability/PDCoverAbility.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Characters/PDPlayerCharacter.h"
#include "Cover/PDCoverBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"

UPDCoverAbility::UPDCoverAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ActivationBlockedTags.AddTag(PDGameplayTags::Cover_Active);
    ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
}

void UPDCoverAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(GetPDCharacter());
    if (!Player)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    APDCoverBase* Cover = Player->GetCoverCandidate();
    if (!Cover || !Cover->IsUsable() || Cover->IsOccupied())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CurrentCover = Cover;
    Cover->TryOccupy(Player);
    Cover->OnCoverDestroyed.BindUObject(this, &UPDCoverAbility::NotifyExitCover);

    Player->GetCharacterMovement()->DisableMovement();

    const FVector SnapLoc = Cover->GetSnapLocation(Player);
    const FRotator SnapRot = Cover->GetSnapRotation(Player);

    BP_OnEnterCover(Cover, SnapLoc, SnapRot);
}

void UPDCoverAbility::FinishEnterCover()
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        ASC->AddLooseGameplayTag(PDGameplayTags::Cover_Active);
    ApplyCoverBuff();
}

void UPDCoverAbility::NotifyExitCover()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPDCoverAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        ASC->RemoveLooseGameplayTag(PDGameplayTags::Cover_Active);

    RemoveCoverBuff();

    if (CurrentCover.IsValid())
    {
        CurrentCover->OnCoverDestroyed.Unbind();
        if (APDCharacterBase* Char = GetPDCharacter())
            CurrentCover->Release(Char);
        CurrentCover = nullptr;
    }

    if (APDCharacterBase* Char = GetPDCharacter())
        Char->GetCharacterMovement()->SetDefaultMovementMode();

    BP_OnExitCover();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPDCoverAbility::ApplyCoverBuff()
{
    if (!CoverBuffGE) return;
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;
    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CoverBuffGE, 1.f, Context);
    if (Spec.IsValid())
        CoverBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UPDCoverAbility::RemoveCoverBuff()
{
    if (CoverBuffHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
            ASC->RemoveActiveGameplayEffect(CoverBuffHandle);
        CoverBuffHandle = FActiveGameplayEffectHandle();
    }
}
