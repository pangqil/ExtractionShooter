#include "Weapons/PDRifle.h"

APDRifle::APDRifle()
{
    WeaponType = EWeaponType::Rifle;

    LevelStats.Add({ 20.f, 0.12f, 1500.f, 30, 2.0f, 0.90f }); // Lv1
    LevelStats.Add({ 30.f, 0.10f, 1700.f, 35, 1.8f, 0.93f }); // Lv2
    LevelStats.Add({ 40.f, 0.08f, 2000.f, 40, 1.5f, 0.96f }); // Lv3
}

void APDRifle::Fire_Implementation()
{
    if (!CanFire()) return;

    FHitResult Hit;
    if (PerformLineTrace(Hit))
    {
        ApplyDamage(Hit.GetActor(), GetCurrentStats().Damage);
    }

    PostFire();

    if (CurrentAmmo <= 0) StopFire();
}

void APDRifle::StartFire()
{
    if (bIsFiring) return;
    bIsFiring = true;

    // 즉시 1발
    Fire();

    // 연사 모드면 타이머로 반복
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
        ? EFireMode::Single
        : EFireMode::Auto;

    OnFireModeChanged.Broadcast(FireMode);

    // 연사 중이었으면 타이머 정리
    GetWorldTimerManager().ClearTimer(AutoFireHandle);

    UE_LOG(LogTemp, Log, TEXT("FireMode: %s"),
        FireMode == EFireMode::Auto ? TEXT("Auto") : TEXT("Single"));
}

void APDRifle::OnUnequip_Implementation()
{
    StopFire();
    Super::OnUnequip_Implementation();
}

bool APDRifle::PerformLineTrace(FHitResult& OutHit)
{
    const FWeaponLevelStats& Stats = GetCurrentStats();
    FVector Start = WeaponMesh->GetSocketLocation(MuzzleSocketName);

    float Spread = (1.f - Stats.Accuracy) * 100.f;
    FVector RandDir = FVector(
        FMath::RandRange(-Spread, Spread),
        FMath::RandRange(-Spread, Spread), 0.f);

    FVector End = Start + (WeaponMesh->GetForwardVector() + RandDir) * Stats.Range;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(GetWeaponOwner());

    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 1.f);

    return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Pawn, Params);
}