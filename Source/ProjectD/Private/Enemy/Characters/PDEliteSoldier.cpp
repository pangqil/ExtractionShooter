#include "Enemy/Characters/PDEliteSoldier.h"

#include "Components/SkeletalMeshComponent.h"
#include "Cover/PDCoverBase.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

APDEliteSoldier::APDEliteSoldier()
{
	// 풀오토 자동 점화는 비활성 — 발사는 SetPeeking 으로만.
	bAutoFireOnAttackRequested = false;
}

void APDEliteSoldier::SetInCover(APDCoverBase* NewCover)
{
	APDCoverBase* Prev = CurrentCover.Get();
	if (Prev == NewCover) return;

	// 피크 중이었다면 먼저 종료(발사 루프 정지 보장).
	if (bIsPeeking)
	{
		SetPeeking(false);
	}

	if (Prev)
	{
		Prev->Release(this);
	}

	CurrentCover = NewCover;
	bIsInCover = (NewCover != nullptr);

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
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ThrowGrenadeAt: GrenadeClass 미지정 — BP override 또는 GrenadeClass 슬롯 설정 필요."),
			*GetName());
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

	// 궤적/폭발/데미지는 GrenadeClass 측 책임(ProjectileMovementComponent 등).
	World->SpawnActor<AActor>(GrenadeClass, SpawnLoc, SpawnRot, Params);
}

void APDEliteSoldier::OnEnterState_Dead()
{
	// 부모(APDSoldier::OnEnterState_Dead) 가 fire 루프 정지 + 무기 정리를 수행하기 전 cover 점유 해제.
	if (CurrentCover.IsValid())
	{
		SetInCover(nullptr);
	}

	Super::OnEnterState_Dead();
}
