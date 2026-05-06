#include "Weapons/PDWeaponBase.h"

#include "Animation/AnimInstance.h"
#include "Characters/PDPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interfaces/PDDamageable.h"

APDWeaponBase::APDWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
}

// ─── 공통 헬퍼 ──────────────────────────────────────────────────

const FWeaponLevelStats& APDWeaponBase::GetCurrentStats() const
{
	// LevelStats 가 비어있을 때 크래시 방지용 static fallback
	static FWeaponLevelStats DefaultStats;
	if (LevelStats.IsEmpty()) return DefaultStats;

	int32 Idx = FMath::Clamp(CurrentLevel - 1, 0, LevelStats.Num() - 1);
	return LevelStats[Idx];
}

bool APDWeaponBase::CanFire() const
{
	return CurrentAmmo > 0 && !bIsReloading;
}

void APDWeaponBase::PostFire()
{
	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
}

void APDWeaponBase::ApplyDamage(AActor* Target, float DamageAmount)
{
	if (!Target || !Target->Implements<UPDDamageable>()) return;

	FPDDamageInfo Info;
	Info.BaseDamage  = DamageAmount;
	Info.Instigator  = WeaponOwner;

	IPDDamageable::Execute_ApplyDamage(Target, Info);
}

void APDWeaponBase::FinishReload()
{
	bIsReloading = false;
	if (LevelStats.IsValidIndex(CurrentLevel - 1))
		CurrentAmmo = LevelStats[CurrentLevel - 1].MaxAmmo;
}

void APDWeaponBase::PlayWeaponMontage(UAnimMontage* Montage)
{
	if (!Montage || !WeaponMesh) return;
	if (UAnimInstance* AI = WeaponMesh->GetAnimInstance())
		AI->Montage_Play(Montage);
}

void APDWeaponBase::StopWeaponMontage(UAnimMontage* Montage)
{
	if (!Montage || !WeaponMesh) return;
	if (UAnimInstance* AI = WeaponMesh->GetAnimInstance())
		AI->Montage_Stop(0.25f, Montage);
}

void APDWeaponBase::BindMontageEndedForReload(UAnimMontage* Montage)
{
	if (!Montage || !WeaponMesh) return;
	UAnimInstance* AI = WeaponMesh->GetAnimInstance();
	if (!AI) return;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindLambda([this](UAnimMontage* /*M*/, bool bInterrupted)
	{
		if (!bInterrupted)
			FinishReload();
		else
			bIsReloading = false;
	});
	AI->Montage_SetEndDelegate(EndDelegate, Montage);
}

// ─── BlueprintNativeEvent 기본 구현 ─────────────────────────────

void APDWeaponBase::Fire_Implementation()
{
	// 자식에서 오버라이드
}

void APDWeaponBase::Reload_Implementation()
{
	// 자식에서 오버라이드
}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	WeaponOwner = NewOwner;
	// BeginPlay 이후 최초 장착 시 탄 채우기
	if (CurrentAmmo == 0 && !LevelStats.IsEmpty())
		CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDWeaponBase::OnUnequip_Implementation()
{
	WeaponOwner = nullptr;
}

void APDWeaponBase::EjectShell_Implementation()       {}
void APDWeaponBase::DropMagazine_Implementation()     {}
void APDWeaponBase::AttachNewMagazine_Implementation(){}
void APDWeaponBase::OnBoltPulled_Implementation()     {}
void APDWeaponBase::OnBoltReleased_Implementation()   {}
void APDWeaponBase::OnShellInserted_Implementation()  {}

// ─── IPDInteractable ────────────────────────────────────────────

void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
	if (APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor))
	{
		Player->PickupWeapon(this);
	}
}
