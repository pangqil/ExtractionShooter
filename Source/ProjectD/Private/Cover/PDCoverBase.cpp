#include "Cover/PDCoverBase.h"
#include "Component/PDCoverComponent.h"
#include "Components/StaticMeshComponent.h"

APDCoverBase::APDCoverBase()
{
	CoverMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverMesh"));
}

void APDCoverBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP=MaxHP;
}

void APDCoverBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateSlots();
}

FCoverSlot* APDCoverBase::FindBestSlot(AActor* Requster)
{
	if (!IsUsable()) return nullptr;
	
	FCoverSlot* Best=nullptr;
	float BestDist=FLT_MAX;
	const FVector RequesterLoc=Requster->GetActorLocation();
	
	for (FCoverSlot& Slot :Slots)
	{
		if (Slot.bOccupied) continue;
		const FVector WorldSlotPos=GetActorTransform().TransformPosition(Slot.LocalOffset);
		const float Dist=FVector::DistSquared(RequesterLoc, WorldSlotPos);
		if (Dist<BestDist)
		{
			BestDist=Dist;
			Best=&Slot;
		}
	}
	return Best;
}

bool APDCoverBase::OccupySlot(AActor* Requester, FCoverSlot* Slot)
{
	if (!Slot||Slot->bOccupied) return false;
	Slot->bOccupied=true;
	Slot->Occupant=Requester;
	return true;
}

void APDCoverBase::ReleaseSlot(AActor* Requester)
{
	for (FCoverSlot& Slot:Slots)
	{
		if (Slot.Occupant==Requester)
		{
			Slot.bOccupied=false;
			Slot.Occupant=nullptr;
			return;
		}
	}
}

void APDCoverBase::TakeCoverDamage(float Damage)
{
	if (CoverState==ECoverState::Destroyed) return;
	CurrentHP=FMath::Max(CurrentHP-Damage, 0.f);

	if (CurrentHP<=0.f)
		SetCoverState(ECoverState::Destroyed);
	else if (CurrentHP<=MaxHP*DamagedThreshold)
		SetCoverState(ECoverState::Damaged);
}

void APDCoverBase::SetCoverState(ECoverState NewState)
{
	if (CoverState==NewState) return;
	CoverState=NewState;
	if (CoverState==ECoverState::Destroyed)
		OnDestroyed_Internal();
}

void APDCoverBase::OnDestroyed_Internal()
{
	NotifyOccupantsDestroyed();
}

void APDCoverBase::NotifyOccupantsDestroyed()
{
	for (FCoverSlot& Slot:Slots)
	{
		if (!Slot.bOccupied||!Slot.Occupant.IsValid()) continue;
		if (UPDCoverComponent* CoverComp=Slot.Occupant->FindComponentByClass<UPDCoverComponent>())
			CoverComp->ForceExitCover();
		Slot.bOccupied=false;
		Slot.Occupant=nullptr;
	}
}

void APDCoverBase::GenerateSlots()
{
	Slots.Empty();
	if (!CoverMesh) return;

	const FBoxSphereBounds LocalBounds=CoverMesh->CalcLocalBounds();
	const FVector Extent=LocalBounds.BoxExtent;

	const TArray<FVector> Directions={
		FVector( 1.f,  0.f, 0.f),
		FVector(-1.f,  0.f, 0.f),
		FVector( 0.f,  1.f, 0.f),
		FVector( 0.f, -1.f, 0.f),
	};

	for (const FVector& Dir:Directions)
	{
		FCoverSlot NewSlot;
		const float ExtentAlongDir=FMath::Abs(Dir.X)*Extent.X
								  +FMath::Abs(Dir.Y)*Extent.Y;
		NewSlot.LocalOffset=Dir*(ExtentAlongDir+CharacterClearance);
		NewSlot.LocalOffset.Z=0.f;
		NewSlot.FacingRotation=Dir.Rotation();
		Slots.Add(NewSlot);
	}
}
