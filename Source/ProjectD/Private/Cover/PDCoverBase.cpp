#include "Cover/PDCoverBase.h"
#include "Component/PDCoverComponent.h"
#include "Components/StaticMeshComponent.h"

APDCoverBase::APDCoverBase()
{
	CoverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverMesh"));
	SetRootComponent(CoverMesh);
	CoverMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void APDCoverBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
}

FVector APDCoverBase::GetSnapLocation(AActor* Requester) const
{
	if (!CoverMesh || !Requester) return GetActorLocation();
	
	const FVector CoverCenter = CoverMesh->GetComponentLocation();
	
	FVector ToCharacter = (Requester->GetActorLocation() - CoverCenter);
	ToCharacter.Z = 0.f;
	ToCharacter = ToCharacter.GetSafeNormal();
	
	const FBoxSphereBounds Bounds = CoverMesh->CalcBounds(CoverMesh->GetComponentTransform());
	const FVector Extent = Bounds.BoxExtent;
	const float HalfExtent = FMath::Abs(ToCharacter.X) * Extent.X
	                        + FMath::Abs(ToCharacter.Y) * Extent.Y;

	FVector SnapPos = CoverCenter + ToCharacter * (HalfExtent + CharacterClearance);
	SnapPos.Z = Requester->GetActorLocation().Z; // 캐릭터 Z 높이 유지

	return SnapPos;
}

FRotator APDCoverBase::GetSnapRotation(AActor* Requester) const
{
	if (!Requester) return GetActorRotation();
	
	FVector OutDir = (Requester->GetActorLocation() - GetActorLocation());
	OutDir.Z = 0.f;
	return OutDir.GetSafeNormal().Rotation();
}

bool APDCoverBase::TryOccupy(AActor* Requester)
{
	if (Occupant.IsValid()) return false;
	Occupant = Requester;
	return true;
}

void APDCoverBase::Release(AActor* Requester)
{
	if (Occupant == Requester)
		Occupant = nullptr;
}

void APDCoverBase::TakeCoverDamage(float Damage)
{
	if (CoverState == ECoverState::Destroyed) return;
	CurrentHP = FMath::Max(CurrentHP - Damage, 0.f);

	if (CurrentHP <= 0.f)
		SetCoverState(ECoverState::Destroyed);
	else if (CurrentHP <= MaxHP * DamagedThreshold)
		SetCoverState(ECoverState::Damaged);
}

void APDCoverBase::SetCoverState(ECoverState NewState)
{
	if (CoverState == NewState) return;
	CoverState = NewState;
	if (CoverState == ECoverState::Destroyed)
		OnDestroyed_Internal();
}

void APDCoverBase::OnDestroyed_Internal()
{
	if (!Occupant.IsValid()) return;

	if (UPDCoverComponent* CoverComp = Occupant->FindComponentByClass<UPDCoverComponent>())
		CoverComp->ForceExitCover();

	Occupant = nullptr;
}
