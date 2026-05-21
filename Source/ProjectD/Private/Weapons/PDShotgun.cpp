#include "Weapons/PDShotgun.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"

APDShotgun::APDShotgun()
{
    WeaponType = EWeaponType::Shotgun;

    LevelStats.Add({ 15.f, 0.80f,  800.f, 6, 2.5f, 0.80f });
    LevelStats.Add({ 20.f, 0.75f,  900.f, 8, 2.2f, 0.83f });
    LevelStats.Add({ 28.f, 0.70f, 1000.f, 8, 2.0f, 0.87f });
}

void APDShotgun::Fire_Implementation()
{
    if (!HasAuthority()) return;
    if (!CanFire()) return;

    const FVector MuzzleLoc = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : GetActorLocation();


    ExecuteFireCue(MuzzleLoc, FVector::ZeroVector);


    TArray<FHitResult> Hits;
    PerformPelletTraces(Hits);


    TSet<AActor*> DamagedActors;
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor) continue;

        if (!DamagedActors.Contains(HitActor))
        {
            DamagedActors.Add(HitActor);
            ApplyDamage(HitActor, GetCurrentStats().Damage, Hit);
        }

        ExecuteImpactCue(Hit);
    }

    PlayWeaponMontage(FireMontage);
    PostFire();
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

    APlayerController* PC = nullptr;
    if (APawn* OwnerPawn = Cast<APawn>(WeaponOwnerActor))
        PC = Cast<APlayerController>(OwnerPawn->GetController());

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector Forward = GetAimDirectionFromOwner(Start);

    float TraceLength = GetCurrentStats().Range;
    if (PC)
    {
        FVector AimLocation;
        if (const APDPlayerController* PDPC = Cast<APDPlayerController>(PC);
            PDPC && PDPC->GetCachedAimWorldLocation(AimLocation))
        {
            TraceLength = FVector::Dist(Start, AimLocation);
        }
        else
        {
            FHitResult CursorHit;
            if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
            {
                TraceLength = FVector::Dist(Start, CursorHit.Location);
            }
        }
    }

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);
    Params.bTraceComplex = true;

    const int32 Pellets = GetCurrentPelletCount();
    for (int32 i = 0; i < Pellets; ++i)
    {
        FVector RandDir = FMath::VRandCone(Forward, FMath::DegreesToRadians(SpreadAngle));
        FVector End     = Start + RandDir * TraceLength;

        FHitResult Hit;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

        if (bHit) OutHits.Add(Hit);
    }
}
