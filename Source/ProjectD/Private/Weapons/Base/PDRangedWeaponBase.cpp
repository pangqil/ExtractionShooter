#include "Weapons/Base/PDRangedWeaponBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "Animation/AnimInstance.h"
#include "Interfaces/PDDamageable.h"
#include "Items/PDInventoryComponent.h"
#include "Kismet/GameplayStatics.h"

APDRangedWeaponBase::APDRangedWeaponBase()
{
}

void APDRangedWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (LevelStats.IsValidIndex(0))
		CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDRangedWeaponBase::Fire_Implementation() {}

void APDRangedWeaponBase::Reload_Implementation()
{
	if (bIsReloading) return;
	if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;
	if (!HasAmmoToReload()) return;

	bIsReloading = true;

	if (ReloadMontage && WeaponMesh && WeaponMesh->GetAnimInstance())
	{
		PlayWeaponMontage(ReloadMontage);
		BindMontageEndedForReload(ReloadMontage);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			ReloadHandle, this,
			&APDRangedWeaponBase::FinishReload,
			GetCurrentStats().ReloadTime, false);
	}
}

void APDRangedWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	Super::OnEquip_Implementation(NewOwner);

	if (CurrentAmmo == 0 && !LevelStats.IsEmpty())
		CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDRangedWeaponBase::OnUnequip_Implementation()
{
	Super::OnUnequip_Implementation();

	GetWorldTimerManager().ClearTimer(FireCooldownHandle);
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	bIsReloading = false;
	bCanFire = true;
}

bool APDRangedWeaponBase::CanFire() const
{
	return bCanFire && !bIsReloading && CurrentAmmo > 0;
}

void APDRangedWeaponBase::ApplyDamage(AActor* HitActor, float DamageAmount)
{
	if (!HitActor) return;
	if (!HitActor->Implements<UPDDamageable>()) return;
	if (IPDDamageable::Execute_GetCurrentHealth(HitActor) <= 0.f) return;

	FPDDamageInfo DamageInfo;
	DamageInfo.BaseDamage = DamageAmount;
	DamageInfo.Instigator = GetWeaponOwner();
	DamageInfo.DamageTypeClass = nullptr;

	IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
}

void APDRangedWeaponBase::PostFire()
{
	--CurrentAmmo;
	bCanFire = false;
	OnWeaponFired.Broadcast(this);

	ApplyRecoil();

	GetWorldTimerManager().SetTimer(
		FireCooldownHandle, this,
		&APDRangedWeaponBase::ResetFireCooldown,
		GetCurrentStats().FireRate, false);
}

void APDRangedWeaponBase::ResetFireCooldown()
{
	bCanFire = true;
}

void APDRangedWeaponBase::CancelReload()
{
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	StopWeaponMontage(ReloadMontage);
	bIsReloading = false;
}

void APDRangedWeaponBase::FinishReload()
{
	const int32 MaxAmmo = GetCurrentStats().MaxAmmo;
	const int32 Needed = MaxAmmo - CurrentAmmo;

	if (!AmmoItemID.IsNone())
	{
		UPDInventoryComponent* Inv = GetOwnerInventory();
		if (Inv)
		{
			const int32 ToAdd = FMath::Min(Needed, GetAvailableAmmoCount());
			if (ToAdd > 0)
			{
				Inv->RemoveItem(AmmoItemID, ToAdd);
				CurrentAmmo += ToAdd;
			}
		}
		else { CurrentAmmo = MaxAmmo; }
	}
	else { CurrentAmmo = MaxAmmo; }

	bIsReloading = false;
	OnWeaponReloaded.Broadcast(this);
}

void APDRangedWeaponBase::PlayWeaponMontage(UAnimMontage* Montage, FName StartSection)
{
	if (!Montage) return;
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	AnimInst->Montage_Play(Montage);
	if (StartSection != NAME_None)
		AnimInst->Montage_JumpToSection(StartSection, Montage);
}

bool APDRangedWeaponBase::IsPlayingMontage(UAnimMontage* Montage) const
{
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	return AnimInst ? AnimInst->Montage_IsPlaying(Montage) : false;
}

void APDRangedWeaponBase::StopWeaponMontage(UAnimMontage* Montage)
{
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (AnimInst && Montage)
		AnimInst->Montage_Stop(0.15f, Montage);
}

void APDRangedWeaponBase::BindMontageEndedForReload(UAnimMontage* Montage)
{
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	FOnMontageEnded EndedDelegate;
	EndedDelegate.BindUObject(this, &APDRangedWeaponBase::OnReloadMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndedDelegate, Montage);
}

void APDRangedWeaponBase::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
		FinishReload();
	else
		bIsReloading = false;
}

void APDRangedWeaponBase::ApplyRecoil()
{
	if (FireCameraShakeClass)
	{
		if (APlayerController* PC = GetOwnerPlayerController())
			PC->PlayerCameraManager->StartCameraShake(FireCameraShakeClass);
	}

	CurrentRecoilSpread = FMath::Min(
		CurrentRecoilSpread + RecoilSpreadPerShot,
		MaxRecoilSpread);

	GetWorldTimerManager().SetTimer(
		SpreadRecoveryHandle, this,
		&APDRangedWeaponBase::TickSpreadRecovery,
		0.016f, true);
}

void APDRangedWeaponBase::TickSpreadRecovery()
{
	CurrentRecoilSpread -= RecoilRecoveryRate * 0.016f;

	if (CurrentRecoilSpread <= 0.f)
	{
		CurrentRecoilSpread = 0.f;
		GetWorldTimerManager().ClearTimer(SpreadRecoveryHandle);
	}
}

void APDRangedWeaponBase::PlayFireEffects()
{
	if (MuzzleFlashEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashEffect,
			WeaponMesh,
			MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}

	if (FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			FireSound,
			WeaponMesh,
			MuzzleSocketName);
	}
}

void APDRangedWeaponBase::SpawnTracerEffect(const FVector& Start, const FVector& End)
{
	if (!TracerEffect || !GetWorld()) return;

	FVector Dir = (End - Start).GetSafeNormal();
	float Distance = FVector::Dist(Start, End);
	float Speed = Distance / 0.05f;

	UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(), TracerEffect, Start, Dir.Rotation());

	if (PSC)
	{
		PSC->SetVectorParameter(FName("InitialVelocity"), Dir * Speed);
		PSC->SetFloatParameter(FName("InitialSpeed"), Speed);
	}
}

APlayerController* APDRangedWeaponBase::GetOwnerPlayerController() const
{
	if (!WeaponOwner.IsValid()) return nullptr;
	return Cast<APlayerController>(WeaponOwner->GetInstigatorController());
}

UPDInventoryComponent* APDRangedWeaponBase::GetOwnerInventory() const
{
	if (!WeaponOwner.IsValid()) return nullptr;
	return WeaponOwner->FindComponentByClass<UPDInventoryComponent>();
}

bool APDRangedWeaponBase::HasAmmoToReload() const
{
	if (AmmoItemID.IsNone()) return true;
	UPDInventoryComponent* Inv = GetOwnerInventory();
	if (!Inv) return true;
	return Inv->HasItem(AmmoItemID, 1);
}

int32 APDRangedWeaponBase::GetAvailableAmmoCount() const
{
	if (AmmoItemID.IsNone()) return INT32_MAX;
	UPDInventoryComponent* Inv = GetOwnerInventory();
	if (!Inv) return INT32_MAX;
	int32 Total = 0;
	for (const FPDInventorySlot& Slot : Inv->Items)
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == AmmoItemID)
			Total += Slot.Quantity;
	return Total;
}
