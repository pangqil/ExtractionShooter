#include "Component/PDCoverComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "Cover/PDCoverBase.h"
#include "Engine/OverlapResult.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameplayTag/PDGameplayTags.h"

UPDCoverComponent::UPDCoverComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
}

ACharacter* UPDCoverComponent::GetOwnerCharacter() const
{
    return Cast<ACharacter>(GetOwner());
}

UAbilitySystemComponent* UPDCoverComponent::GetASC() const
{
    return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

void UPDCoverComponent::TryEnterCover()
{
    if (IsInCover()) { ExitCover(); return; }

    ACharacter* Owner=GetOwnerCharacter();
    if (!IsValid(Owner)) return;

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        Owner->GetActorLocation(),
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_WorldStatic),
        FCollisionShape::MakeSphere(CoverSearchRadius),
        Params);

    APDCoverBase* BestCover=nullptr;
    FCoverSlot* BestSlot=nullptr;
    float BestDist=FLT_MAX;

    for (const FOverlapResult& Result:Overlaps)
    {
        APDCoverBase* Cover=Cast<APDCoverBase>(Result.GetActor());
        if (!IsValid(Cover)||!Cover->IsUsable()) continue;

        FCoverSlot* Slot=Cover->FindBestSlot(Owner);
        if (!Slot) continue;

        const FVector WorldSlotPos=Cover->GetActorTransform().TransformPosition(Slot->LocalOffset);
        const float Dist=FVector::DistSquared(Owner->GetActorLocation(), WorldSlotPos);
        if (Dist<BestDist)
        {
            BestDist=Dist;
            BestCover=Cover;
            BestSlot=Slot;
        }
    }

    if (!BestCover||!BestSlot) return;

    CurrentCoverActor=BestCover;

    const FVector TargetPos=BestCover->GetActorTransform().TransformPosition(BestSlot->LocalOffset);
    
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(Owner->GetController(), TargetPos);
    GetWorld()->GetTimerManager().SetTimer(CoverArrivalCheckHandle, [this, TargetPos]()
    {
        ACharacter* Char=GetOwnerCharacter();
        if (!IsValid(Char)||!CurrentCoverActor.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(CoverArrivalCheckHandle);
            return;
        }
        if (FVector::Dist(Char->GetActorLocation(), TargetPos)<CoverArrivalTolerance)
        {
            GetWorld()->GetTimerManager().ClearTimer(CoverArrivalCheckHandle);
            OnCoverArrived();
        }
    }, 0.1f, true);
}

void UPDCoverComponent::OnCoverArrived()
{
    if (!CurrentCoverActor.IsValid()) return;
    CurrentCoverActor->ReleaseSlot(GetOwner());
    ApplyCoverState();
    StartCoverProximityCheck();
}

void UPDCoverComponent::StartCoverProximityCheck()
{
    GetWorld()->GetTimerManager().SetTimer(CoverProximityCheckHandle, [this]()
    {
        if (!CurrentCoverActor.IsValid()) { ExitCover(); return; }

        ACharacter* Char=GetOwnerCharacter();
        if (!IsValid(Char)) { ExitCover(); return; }

        const float Dist=FVector::Dist(Char->GetActorLocation(), CurrentCoverActor->GetActorLocation());
        if (Dist>CoverExitDistance)
            ExitCover();

    }, 0.2f, true);
}

void UPDCoverComponent::ApplyCoverState()
{
    UAbilitySystemComponent* ASC=GetASC();
    if (!ASC||!CurrentCoverActor.IsValid()) return;
    ASC->AddLooseGameplayTag(PDGameplayTags::Cover_Active);
    ASC->AddLooseGameplayTag(CurrentCoverActor->GetCoverTypeTag());
}

void UPDCoverComponent::ExitCover()
{
    GetWorld()->GetTimerManager().ClearTimer(CoverArrivalCheckHandle);
    GetWorld()->GetTimerManager().ClearTimer(CoverProximityCheckHandle);

    ACharacter* Char=GetOwnerCharacter();
    if (IsValid(Char))
        Char->GetController()->StopMovement();

    CurrentCoverActor=nullptr;
    RemoveCoverState();
}

void UPDCoverComponent::ForceExitCover()
{
    ExitCover();
}

void UPDCoverComponent::RemoveCoverState()
{
    UAbilitySystemComponent* ASC=GetASC();
    if (!ASC) return;
    ASC->RemoveLooseGameplayTag(PDGameplayTags::Cover_Active);
    ASC->RemoveLooseGameplayTag(PDGameplayTags::Cover_Type_High);
    ASC->RemoveLooseGameplayTag(PDGameplayTags::Cover_Type_Low);
}