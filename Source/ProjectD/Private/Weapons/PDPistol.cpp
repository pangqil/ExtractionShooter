#include "Weapons/PDPistol.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

APDPistol::APDPistol()
{
	WeaponType = EWeaponType::Pistol;

	// 저댐/고연사/단거리/소탄창 — 보조무기 컨셉
	LevelStats.Add({ 15.f, 0.25f, 1200.f, 12, 1.5f, 0.92f }); // Lv1
	LevelStats.Add({ 20.f, 0.22f, 1300.f, 15, 1.3f, 0.94f }); // Lv2
	LevelStats.Add({ 28.f, 0.20f, 1500.f, 17, 1.2f, 0.96f }); // Lv3
}

void APDPistol::Fire_Implementation()
{
	if (!CanFire()) return;

	FHitResult Hit;
	if (PerformLineTrace(Hit))
		ApplyDamage(Hit.GetActor(), GetCurrentStats().Damage);

	PostFire();

	FTimerHandle T_Shell;
	GetWorldTimerManager().SetTimer(T_Shell, FTimerDelegate::CreateLambda([this]()
		{
			EjectShell();
		}), 0.05f, false);
}

void APDPistol::Reload_Implementation()
{
	if (bIsReloading) return;
	if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;

	bIsReloading = true;

	if (ReloadMontage)
	{
		PlayWeaponMontage(ReloadMontage);
		BindMontageEndedForReload(ReloadMontage);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			ReloadHandle, this,
			&APDWeaponBase::FinishReload,
			GetCurrentStats().ReloadTime, false);
	}
}

bool APDPistol::PerformLineTrace(FHitResult& OutHit)
{
	const FWeaponLevelStats& Stats = GetCurrentStats();
	AActor* WeaponOwnerActor = GetWeaponOwner();
	if (!WeaponOwnerActor) return false;

	APlayerController* PC = Cast<APlayerController>(
		WeaponOwnerActor->GetInstigatorController());

	FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
		? WeaponMesh->GetSocketLocation(MuzzleSocketName)
		: WeaponOwnerActor->GetActorLocation();

	FVector AimDir = GetAimDirectionFromOwner(Start);

	float TraceLength = Stats.Range;
	if (PC)
	{
		FHitResult CursorHit;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
			TraceLength = FVector::Dist(Start, CursorHit.Location);
	}

	const float BaseSpread = (1.f - Stats.Accuracy) * 5.f;
	const float TotalSpread = FMath::DegreesToRadians(BaseSpread + CurrentRecoilSpread);
	const FVector ShootDir = FMath::VRandCone(AimDir, TotalSpread);
	const FVector End = Start + ShootDir * TraceLength;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(WeaponOwnerActor);
	Params.bTraceComplex = true;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Pawn, Params);
	DrawDebugLine(GetWorld(), Start, bHit ? OutHit.Location : End,
		bHit ? FColor::Red : FColor::Green, false, 1.f, 0, 1.f);

	return bHit;
}
