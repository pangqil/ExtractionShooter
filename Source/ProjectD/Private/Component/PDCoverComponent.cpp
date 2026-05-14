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
	PrimaryComponentTick.bCanEverTick = false;
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

	ACharacter* Owner = GetOwnerCharacter();
	if (!IsValid(Owner)) return;

	// 주변 APDCoverBase 탐색
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

	APDCoverBase* BestCover = nullptr;
	float BestDist = FLT_MAX;

	for (const FOverlapResult& Result : Overlaps)
	{
		APDCoverBase* Cover = Cast<APDCoverBase>(Result.GetActor());
		if (!IsValid(Cover) || !Cover->IsUsable() || Cover->IsOccupied()) continue;

		const float Dist = FVector::DistSquared(Owner->GetActorLocation(), Cover->GetActorLocation());
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestCover = Cover;
		}
	}

	if (!BestCover) return;

	CurrentCoverActor = BestCover;

	// 캐릭터 위치 기준으로 스냅 위치/회전 자동 계산
	SnapLocation = BestCover->GetSnapLocation(Owner);
	SnapRotation = BestCover->GetSnapRotation(Owner);

	if (Owner->GetController())
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(Owner->GetController(), SnapLocation);

	// 도착 체크 타이머
	TWeakObjectPtr<UPDCoverComponent> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimer(CoverArrivalCheckHandle, [WeakThis]()
	{
		if (!WeakThis.IsValid()) return;

		UPDCoverComponent* Comp = WeakThis.Get();
		ACharacter* Char = Comp->GetOwnerCharacter();

		if (!IsValid(Char) || !Comp->CurrentCoverActor.IsValid())
		{
			Comp->GetWorld()->GetTimerManager().ClearTimer(Comp->CoverArrivalCheckHandle);
			return;
		}

		if (FVector::Dist(Char->GetActorLocation(), Comp->SnapLocation) < Comp->CoverArrivalTolerance)
		{
			Comp->GetWorld()->GetTimerManager().ClearTimer(Comp->CoverArrivalCheckHandle);
			Comp->OnCoverArrived();
		}
	}, 0.1f, true);
}

void UPDCoverComponent::OnCoverArrived()
{
	if (!CurrentCoverActor.IsValid()) return;

	ACharacter* Char = GetOwnerCharacter();
	if (!IsValid(Char)) return;

	Char->SetActorLocation(SnapLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Char->SetActorRotation(SnapRotation);

	CurrentCoverActor->TryOccupy(Char);

	LockMovement();
	ApplyCoverState();
}

void UPDCoverComponent::LockMovement()
{
	ACharacter* Char = GetOwnerCharacter();
	if (!IsValid(Char)) return;
	Char->GetCharacterMovement()->DisableMovement();
}

void UPDCoverComponent::UnlockMovement()
{
	ACharacter* Char = GetOwnerCharacter();
	if (!IsValid(Char)) return;
	Char->GetCharacterMovement()->SetDefaultMovementMode();
}

void UPDCoverComponent::ApplyCoverState()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC) return;

	ASC->AddLooseGameplayTag(PDGameplayTags::Cover_Active);

	if (!CoverBuff) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CoverBuff, 1.f, Context);
	if (Spec.IsValid())
		ActiveCoverBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UPDCoverComponent::ExitCover()
{
	GetWorld()->GetTimerManager().ClearTimer(CoverArrivalCheckHandle);

	ACharacter* Char = GetOwnerCharacter();
	if (IsValid(Char))
	{
		Char->GetController()->StopMovement();

		if (CurrentCoverActor.IsValid())
			CurrentCoverActor->Release(Char);
	}

	CurrentCoverActor = nullptr;
	SnapLocation = FVector::ZeroVector;
	SnapRotation = FRotator::ZeroRotator;

	UnlockMovement();
	RemoveCoverState();
}

void UPDCoverComponent::ForceExitCover()
{
	ExitCover();
}

void UPDCoverComponent::RemoveCoverState()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC) return;

	ASC->RemoveLooseGameplayTag(PDGameplayTags::Cover_Active);

	if (ActiveCoverBuffHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ActiveCoverBuffHandle);
		ActiveCoverBuffHandle = FActiveGameplayEffectHandle();
	}
}
