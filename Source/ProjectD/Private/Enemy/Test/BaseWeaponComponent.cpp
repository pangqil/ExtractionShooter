// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Test/BaseWeaponComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Enemy/Characters/PDSoldier.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/PDProjectile.h"

UBaseWeaponComponent::UBaseWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WeaponMesh(TEXT("/Game/Assets/Meshes/SM_Rifle.SM_Rifle"));
	if (WeaponMesh.Succeeded())
	{
		SetStaticMesh(WeaponMesh.Object);
	}
}

void UBaseWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	APDSoldier* Soldier = Cast<APDSoldier>(GetOwner());
	if (!Soldier)
	{
		// Soldier 가 아닌 액터에 부착되면 발사/부착 모두 의미 없음 — 메시만 표시하고 종료.
		return;
	}

	// Soldier 메시의 hand_right 소켓에 부착. 소켓 부재 시 silent fallback 방지 위해 가드.
	if (USkeletalMeshComponent* SoldierMesh = Soldier->GetMesh())
	{
		if (!WeaponAttachSocket.IsNone() && SoldierMesh->DoesSocketExist(WeaponAttachSocket))
		{
			AttachToComponent(
				SoldierMesh,
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				WeaponAttachSocket
			);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] AttachSocket '%s' not found on Soldier mesh — weapon stays at root."),
				*GetName(), *WeaponAttachSocket.ToString());
		}
	}

	// 책임 인계: 본 컴포넌트가 발사 담당 → Soldier 내장 자동 발사 비활성.
	Soldier->SetAutoFireProjectile(false);

	// CombatComponent.OnAttackRequested 자동 구독.
	if (UPDCombatComponent* Combat = Soldier->GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &UBaseWeaponComponent::HandleAttackRequested);
	}
}

void UBaseWeaponComponent::HandleAttackRequested(AActor* Target)
{
	Shoot(Target);
}

void UBaseWeaponComponent::Shoot(AActor* Target)
{
	if (!BulletClass || !Target)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 머즐 위치: 무기 메시의 'Muzzle' 소켓 → 없으면 컴포넌트 위치 + 로컬 오프셋.
	FVector SpawnLocation;
	if (!MuzzleSocketName.IsNone() && DoesSocketExist(MuzzleSocketName))
	{
		SpawnLocation = GetSocketLocation(MuzzleSocketName);
	}
	else
	{
		SpawnLocation = GetComponentLocation() + GetComponentRotation().RotateVector(MuzzleOffset);
	}

	// 발사 방향: 머즐 → 타겟. (Pitch 까지 자동 보정됨)
	const FVector ToTarget = Target->GetActorLocation() - SpawnLocation;
	const FRotator SpawnRotation = ToTarget.IsNearlyZero() ? Owner->GetActorRotation() : ToTarget.Rotation();

	FActorSpawnParameters SpawnParams;
	// 머즐이 캐릭터 캡슐 안쪽이라도 silent 실패하지 않도록 AlwaysSpawn.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);

	APDProjectile* Projectile = World->SpawnActor<APDProjectile>(BulletClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile)
	{
		// 자해 필터링 + 데미지 주입 — InitProjectile 미호출이 가장 흔한 버그.
		Projectile->InitProjectile(BulletDamage, Owner, bBulletCanPenetrate);
	}
}
