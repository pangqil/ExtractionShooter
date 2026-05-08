#include "Weapons/PDRifle.h"
#include "Core/PDPlayerController.h"
#include "DrawDebugHelpers.h"

APDRifle::APDRifle()
{
    WeaponType = EWeaponType::Rifle;

    DefaultFOV = 90.f;
    ZoomedFOV = 70.f;

    LevelStats.Add({ 20.f, 0.12f, 1500.f, 30, 2.0f, 0.90f }); // Lv1
    LevelStats.Add({ 28.f, 0.10f, 1700.f, 35, 1.8f, 0.93f }); // Lv2
    LevelStats.Add({ 38.f, 0.08f, 2000.f, 40, 1.5f, 0.96f }); // Lv3
}

void APDRifle::Fire_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("Fire called! CanFire: %d, Ammo: %d"),
        CanFire(), CurrentAmmo);

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

    if (CurrentAmmo <= 0) StopFire();
}

void APDRifle::Reload_Implementation()
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
        // 몽타주 미설정 시 폴백
        GetWorldTimerManager().SetTimer(
            ReloadHandle, this,
            &APDWeaponBase::FinishReload,
            GetCurrentStats().ReloadTime, false);
    }
}

void APDRifle::StartFire()
{
    if (bIsFiring) return;
    bIsFiring = true;

    Fire();

    if (FireMode == EFireMode::Auto)
    {
        GetWorldTimerManager().SetTimer(
            AutoFireHandle, this,
            &APDRifle::Fire_Implementation,
            GetCurrentStats().FireRate, true);
    }
}

void APDRifle::StopFire()
{
    bIsFiring = false;
    GetWorldTimerManager().ClearTimer(AutoFireHandle);
}

void APDRifle::ToggleFireMode()
{
    FireMode = (FireMode == EFireMode::Auto)
        ? EFireMode::Single : EFireMode::Auto;

    GetWorldTimerManager().ClearTimer(AutoFireHandle);
    OnFireModeChanged.Broadcast(FireMode);
}

void APDRifle::OnUnequip_Implementation()
{
    StopFire();
    Super::OnUnequip_Implementation();
}

bool APDRifle::PerformLineTrace(FHitResult& OutHit)
{
    const FWeaponLevelStats& Stats = GetCurrentStats();

    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return false;

    APlayerController* PC = Cast<APlayerController>(
        WeaponOwnerActor->GetInstigatorController());

    // 머즐 소켓 있으면 소켓 위치, 없으면 플레이어 위치
    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector Forward = WeaponOwnerActor->GetActorForwardVector();

    if (PC)
    {
        FHitResult CursorHit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
        {
            FVector Dir = CursorHit.Location - Start; // Start 기준으로 방향 계산
            Dir.Z = 0.f;
            if (!Dir.IsNearlyZero())
                Forward = Dir.GetSafeNormal();
        }
    }

    float SpreadAngle = (1.f - Stats.Accuracy) * 15.f;
    FVector ShootDir = FMath::VRandCone(Forward, FMath::DegreesToRadians(SpreadAngle));
    ShootDir.Z = 0.f;
    ShootDir.Normalize();

    FVector End = Start + ShootDir * Stats.Range;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);

    bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Pawn, Params);

    if (bHit)
    {
        DrawDebugLine(GetWorld(), Start, OutHit.Location, FColor::Red, false, 1.f, 0, 1.f);
        DrawDebugSphere(GetWorld(), OutHit.Location, 10.f, 8, FColor::Red, false, 1.f);
    }
    else
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f, 0, 1.f);
    }

    return bHit;
}