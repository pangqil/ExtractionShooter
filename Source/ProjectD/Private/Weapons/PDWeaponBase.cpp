#include "Weapons/PDWeaponBase.h"
#include "Characters/PDPlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

APDWeaponBase::APDWeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetupAttachment(RootComponent);
    PickupCollision->SetSphereRadius(80.f);
    PickupCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    // 처음엔 비활성화 (장착 상태)
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDWeaponBase::BeginPlay()
{
    Super::BeginPlay();
    if (LevelStats.IsValidIndex(0))
        CurrentAmmo = LevelStats[0].MaxAmmo;

    if (bIsDropped)
    {
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
}

// 핵심 액션

void APDWeaponBase::Fire_Implementation() // 자식 클래스에서 구현
{
    if (MuzzleFlashFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), MuzzleFlashFX,
            WeaponMesh->GetSocketLocation(MuzzleSocketName),
            WeaponMesh->GetSocketRotation(MuzzleSocketName));
    }
}

void APDWeaponBase::Reload_Implementation()
{
    if (bIsReloading) return;
    if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;

    bIsReloading = true;
    GetWorldTimerManager().SetTimer(
        ReloadHandle, this,
        &APDWeaponBase::FinishReload,
        GetCurrentStats().ReloadTime, false);
}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
    WeaponOwner = NewOwner;
    SetOwner(NewOwner);
    WeaponMesh->SetSimulatePhysics(false);
}

void APDWeaponBase::OnUnequip_Implementation()
{
    WeaponOwner = nullptr;
    GetWorldTimerManager().ClearTimer(FireCooldownHandle);
    GetWorldTimerManager().ClearTimer(ReloadHandle);
    bIsReloading = false;
    bCanFire = true;
}

void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
    // 이미 장착된 무기면 무시
    if (WeaponOwner.IsValid()) return;

    APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
    if (!Player) return;

    Player->PickupWeapon(this);
}

void APDWeaponBase::OnPickupOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

// 레벨 시스템

void APDWeaponBase::UpgradeLevel()
{
    SetLevel(CurrentLevel + 1);
}

void APDWeaponBase::SetLevel(int32 NewLevel)
{
    const int32 MaxLevel = LevelStats.Num();
    CurrentLevel = FMath::Clamp(NewLevel, 1, MaxLevel);
    CurrentAmmo = GetCurrentStats().MaxAmmo;
    OnWeaponLevelChanged.Broadcast(this, CurrentLevel);
}

// 공통 함수

const FWeaponLevelStats& APDWeaponBase::GetCurrentStats() const
{
    return LevelStats[FMath::Clamp(CurrentLevel - 1, 0, LevelStats.Num() - 1)];
}

bool APDWeaponBase::CanFire() const
{
    return bCanFire && !bIsReloading && CurrentAmmo > 0;
}

void APDWeaponBase::ApplyDamage(AActor* HitActor, float DamageAmount)
{
    if (!HitActor) return;
    UGameplayStatics::ApplyDamage(HitActor, DamageAmount, nullptr, this, nullptr);
}

void APDWeaponBase::PostFire()
{
    --CurrentAmmo;
    bCanFire = false;
    OnWeaponFired.Broadcast(this);

    GetWorldTimerManager().SetTimer(
        FireCooldownHandle, this,
        &APDWeaponBase::ResetFireCooldown,
        GetCurrentStats().FireRate, false);
}

void APDWeaponBase::ResetFireCooldown()
{
    bCanFire = true;
}

void APDWeaponBase::FinishReload()
{
    CurrentAmmo = GetCurrentStats().MaxAmmo;
    bIsReloading = false;
    OnWeaponReloaded.Broadcast(this);
}

void APDWeaponBase::SetDropped(bool bDropped)
{
    bIsDropped = bDropped;

    if (bDropped)
    {
        // 바닥에 떨어진 상태 → 충돌 활성화
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        WeaponMesh->SetSimulatePhysics(true);
    }
    else
    {
        // 장착 상태 → 충돌 비활성화
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetSimulatePhysics(false);
    }
}