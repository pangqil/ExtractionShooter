#include "Component/PDInteractionComponent.h"

#include "Interfaces/PDInteractable.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

namespace
{
	bool ShouldIgnoreInteractTarget(const AActor* OwnerActor, const AActor* CandidateActor)
	{
		if (!OwnerActor || !CandidateActor || CandidateActor == OwnerActor)
		{
			return true;
		}

		return CandidateActor->GetOwner() == OwnerActor ||
			CandidateActor->GetAttachParentActor() == OwnerActor;
	}
}

UPDInteractionComponent::UPDInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PollTimerHandle,
			this,
			&UPDInteractionComponent::PollTarget,
			PollInterval,
			true,
			0.f);
	}
}

void UPDInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UPDInteractionComponent::Interact()
{
	AActor* TargetActor = FindInteractTarget();

	if (!TargetActor)
	{
		return;
	}

	if (TargetActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
	{
		IPDInteractable::Execute_Interact(TargetActor, GetOwner());
	}
}

AActor* UPDInteractionComponent::FindInteractTarget() const
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !GetWorld())
	{
		return nullptr;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + OwnerActor->GetActorForwardVector() * InteractDistance;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PDInteractionTrace), false, OwnerActor);
	Params.AddIgnoredActor(OwnerActor);
	TArray<AActor*> AttachedActors;
	OwnerActor->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		Params.AddIgnoredActor(AttachedActor);
	}
	const FCollisionShape Shape = FCollisionShape::MakeSphere(80.f);

	GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		Shape,
		Params
	);

	GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		Start + FVector(0.f, 0.f, 1.f),
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(InteractDistance),
		Params
	);

	AActor* ClosestInteractable = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();

	auto ConsiderInteractable = [&](AActor* HitActor, const FVector& HitLocation)
	{
		if (ShouldIgnoreInteractTarget(OwnerActor, HitActor))
		{
			return;
		}

		if (!HitActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
		{
			return;
		}

		const float DistanceSq = FVector::DistSquared(Start, HitLocation);

		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestInteractable = HitActor;
		}
	};

	for (const FHitResult& Hit : Hits)
	{
		ConsiderInteractable(Hit.GetActor(), Hit.ImpactPoint);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Start,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(InteractDistance),
		Params
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* OverlapActor = Overlap.GetActor())
		{
			ConsiderInteractable(OverlapActor, OverlapActor->GetActorLocation());
		}
	}

	return ClosestInteractable;
}

void UPDInteractionComponent::PollTarget()
{
	AActor* NewTarget = FindInteractTarget();
	AActor* OldTarget = CachedTarget.Get();

	if (NewTarget == OldTarget)
	{
		return;
	}

	CachedTarget = NewTarget;
	OnInteractTargetChanged.Broadcast(NewTarget);
}