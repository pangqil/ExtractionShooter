#include "Enemy/Characters/PDSoldier.h"

#include "Components/SkeletalMeshComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Weapons/PDProjectile.h"

APDSoldier::APDSoldier()
{
	// 디폴트 팀 ID (예: 2 = Hostile). BP 디폴트에서 오버라이드 가능.
	TeamID = 2;
}

void APDSoldier::BeginPlay()
{
	Super::BeginPlay();

	// Senior: CombatComponent 의 OnAttackRequested 는 "공격 의도" 신호.
	//         실제 발사 효과(Projectile/SFX/애님)는 구독 측 책임 — 본 캐릭터가 디폴트 구현 제공.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &APDSoldier::HandleAttackRequested);
	}
}

void APDSoldier::HandleAttackRequested(AActor* Target)
{
	if (!bAutoFireProjectile) return;
	FireProjectile(Target);
}

void APDSoldier::FireProjectile(AActor* Target)
{
	if (!ProjectileClass || !Target)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if(AttackMontage)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Playing attack montage."), *GetName());
		PlayAnimMontage(AttackMontage);
	}

	// 머즐 위치/회전 결정.
	FVector MuzzleLocation = GetActorLocation() + GetActorRotation().RotateVector(MuzzleOffset);
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (!MuzzleSocketName.IsNone() && MeshComp->DoesSocketExist(MuzzleSocketName))
		{
			MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		}
	}


	// Senior: 직사 발사 — 회전은 머즐→타겟 벡터에서 산출. 예측 사격/탄도 보정은
	//         별도 무기 컴포넌트 도입 시 그쪽에서 처리.
	const FVector ToTarget = Target->GetActorLocation() - MuzzleLocation;
	const FRotator MuzzleRotation = ToTarget.IsNearlyZero() ? GetActorRotation() : ToTarget.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	APDProjectile* Projectile = World->SpawnActor<APDProjectile>(ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);
	if (Projectile)
	{
		Projectile->InitProjectile(ProjectileDamage, this, bProjectileCanPenetrate);
	}
}
