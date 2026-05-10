#include "Component/PDCoverComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Cover/PDCoverBase.h"
#include "Engine/OverlapResult.h"
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
    if (IsInCover())
    {
	    ExitCover(); 
    	return;
    }

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

    for (const FOverlapResult& Result : Overlaps)
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
	
    SnapLocation=BestCover->GetActorTransform().TransformPosition(BestSlot->LocalOffset);
    SnapRotation=(BestCover->GetActorRotation().Quaternion()*BestSlot->FacingRotation.Quaternion()).Rotator();
	
    if (Owner->GetController())
    {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(Owner->GetController(), SnapLocation);
    }
	
    TWeakObjectPtr<UPDCoverComponent> WeakThis(this);
    GetWorld()->GetTimerManager().SetTimer(CoverArrivalCheckHandle, [WeakThis]()
    {
       if (!WeakThis.IsValid()) return;
       
       UPDCoverComponent* Comp=WeakThis.Get();
       ACharacter* Char=Comp->GetOwnerCharacter();
       
       if (!IsValid(Char)||!Comp->CurrentCoverActor.IsValid())
       {
          Comp->GetWorld()->GetTimerManager().ClearTimer(Comp->CoverArrivalCheckHandle);
          return;
       }
    	
       if (FVector::Dist(Char->GetActorLocation(), Comp->SnapLocation)<Comp->CoverArrivalTolerance)
       {
          Comp->GetWorld()->GetTimerManager().ClearTimer(Comp->CoverArrivalCheckHandle);
          Comp->OnCoverArrived();
       }
    }, 0.1f, true);
}

void UPDCoverComponent::OnCoverArrived()
{
	if (!CurrentCoverActor.IsValid()) return;

	ACharacter* Char=GetOwnerCharacter();
	if (!IsValid(Char)) return;

	Char->SetActorLocation(SnapLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Char->SetActorRotation(SnapRotation);

	CurrentCoverActor->ReleaseSlot(Char);

	LockMovement();
	ApplyCoverState();
}

void UPDCoverComponent::LockMovement()
{
	ACharacter* Char=GetOwnerCharacter();
	if (!IsValid(Char)) return;
	Char->GetCharacterMovement()->DisableMovement();
}

void UPDCoverComponent::UnlockMovement()
{
	ACharacter* Char=GetOwnerCharacter();
	if (!IsValid(Char)) return;
	Char->GetCharacterMovement()->SetDefaultMovementMode();
}

void UPDCoverComponent::ApplyCoverState()
{
	UAbilitySystemComponent* ASC=GetASC();
	if (!ASC||!CurrentCoverActor.IsValid()) return;

	ASC->AddLooseGameplayTag(PDGameplayTags::Cover_Active);

	if (!CoverBuff) return;

	FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(CoverBuff, 1.f, Context);
	if (Spec.IsValid())
		ActiveCoverBuffHandle=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UPDCoverComponent::ExitCover()
{
	GetWorld()->GetTimerManager().ClearTimer(CoverArrivalCheckHandle);

	ACharacter* Char=GetOwnerCharacter();
	if (IsValid(Char))
		Char->GetController()->StopMovement();

	UnlockMovement();

	CurrentCoverActor=nullptr;
	SnapLocation=FVector::ZeroVector;
	SnapRotation=FRotator::ZeroRotator;

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

	if (ActiveCoverBuffHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ActiveCoverBuffHandle);
		ActiveCoverBuffHandle=FActiveGameplayEffectHandle();
	}
}
