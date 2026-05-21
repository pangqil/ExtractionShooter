#include "Enemy/Characters/PDEliteSoldier.h"

#include "Components/SkeletalMeshComponent.h"
#include "Cover/PDCoverBase.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

APDEliteSoldier::APDEliteSoldier()
{
	bAutoFireOnAttackRequested = false;
}

void APDEliteSoldier::SetInCover(APDCoverBase* NewCover)
{
	APDCoverBase* Prev = CurrentCover.Get();
	if (Prev == NewCover) return;

	if (bIsPeeking)
	{
		SetPeeking(false);
	}

	if (Prev)
	{
		Prev->Release(this);
	}

	CurrentCover = NewCover;
	bIsInCover = NewCover != nullptr;

	if (NewCover)
	{
		BP_OnEnterCover(NewCover);
	}
	else
	{
		BP_OnExitCover();
	}
}

void APDEliteSoldier::SetPeeking(bool bPeek)
{
	if (bIsPeeking == bPeek) return;
	bIsPeeking = bPeek;

	if (bPeek)
	{
		StartContinuousFire();
		BP_OnStartPeek();
	}
	else
	{
		StopContinuousFire();
		BP_OnEndPeek();
	}
}

void APDEliteSoldier::ThrowGrenadeAt_Implementation(const FVector& TargetLocation)
{
	if (!GrenadeClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ThrowGrenadeAt: GrenadeClass is not set."), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	FVector SpawnLoc = GetActorLocation();
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (SkelMesh->DoesSocketExist(GrenadeSpawnSocketName))
		{
			SpawnLoc = SkelMesh->GetSocketLocation(GrenadeSpawnSocketName);
		}
	}

	const FRotator SpawnRot = (TargetLocation - SpawnLoc).Rotation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	World->SpawnActor<AActor>(GrenadeClass, SpawnLoc, SpawnRot, Params);
}

void APDEliteSoldier::OnEnterState_Dead()
{
	if (CurrentCover.IsValid())
	{
		SetInCover(nullptr);
	}

	Super::OnEnterState_Dead();
}
