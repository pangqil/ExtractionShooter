#include "Weapons/PDShotgun.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

APDShotgun::APDShotgun()
{
    WeaponType = EWeaponType::Shotgun;

    LevelStats.Add({ 15.f, 0.80f,  800.f, 6, 2.5f, 0.80f }); // Lv1
    LevelStats.Add({ 20.f, 0.75f,  900.f, 8, 2.2f, 0.83f }); // Lv2
    LevelStats.Add({ 28.f, 0.70f, 1000.f, 8, 2.0f, 0.87f }); // Lv3
}

void APDShotgun::Fire_Implementation()
{
    if (!CanFire()) return;

    TArray<FHitResult> Hits;
    PerformPelletTraces(Hits);

    TSet<AActor*> HitActors;
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && !HitActors.Contains(HitActor))
        {
            HitActors.Add(HitActor);
            ApplyDamage(HitActor, GetCurrentStats().Damage);
        }
    }

    PlayFireEffects();
    PlayWeaponMontage(FireMontage);
    PostFire();
}

void APDShotgun::Reload_Implementation()
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
        GetWorldTimerManager().SetTimer(ReloadHandle, this, &APDRangedWeaponBase::FinishReload, GetCurrentStats().ReloadTime, false);
    }
}

int32 APDShotgun::GetCurrentPelletCount() const
{
    int32 Idx = FMath::Clamp(CurrentLevel - 1, 0, PelletCountPerLevel.Num() - 1);
    return PelletCountPerLevel[Idx];
}

void APDShotgun::PerformPelletTraces(TArray<FHitResult>& OutHits)
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return;

    APlayerController* PC = Cast<APlayerController>(WeaponOwnerActor->GetInstigatorController());

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector Forward = GetAimDirectionFromOwner(Start);

    float TraceLength = GetCurrentStats().Range;
    if (PC)
    {
        FHitResult CursorHit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
            TraceLength = FVector::Dist(Start, CursorHit.Location);
    }

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);
    Params.bTraceComplex = true;

    const int32 Pellets = GetCurrentPelletCount();

    for (int32 i = 0; i < Pellets; ++i)
    {
        FVector RandDir = FMath::VRandCone(Forward, FMath::DegreesToRadians(SpreadAngle));
        FVector End = Start + RandDir * TraceLength;

        FHitResult Hit;
        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

        if (bHit) OutHits.Add(Hit);
        SpawnTracerEffect(Start, bHit ? Hit.Location : End);
    }
}
