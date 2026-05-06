#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDShellActor.h"
#include "Weapons/PDMagazineActor.h"
#include "Characters/PDPlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "NiagaraFunctionLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Interfaces/PDDamageable.h"

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
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetupAttachment(RootComponent);
    PickupCollision->SetSphereRadius(80.f);
    PickupCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
    MagazineMesh->SetupAttachment(WeaponMesh, MagazineSocketName);
    MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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


void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
    if (WeaponOwner.IsValid()) return;

    APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
    if (!Player) return;

    Player->PickupWeapon(this);
    SetDropped(false);
}

void APDWeaponBase::Fire_Implementation() {}

void APDWeaponBase::Reload_Implementation()
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

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	WeaponOwner = NewOwner;
	// BeginPlay 이후 최초 장착 시 탄 채우기
	if (CurrentAmmo == 0 && !LevelStats.IsEmpty())
		CurrentAmmo = LevelStats[0].MaxAmmo;
    WeaponOwner = NewOwner;
    SetOwner(NewOwner);
    WeaponMesh->SetSimulatePhysics(false);
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDWeaponBase::OnUnequip_Implementation()
{
    WeaponOwner = nullptr;
    GetWorldTimerManager().ClearTimer(FireCooldownHandle);
    GetWorldTimerManager().ClearTimer(ReloadHandle);
    bIsReloading = false;
    bCanFire = true;
}

void APDWeaponBase::UpgradeLevel()
{
    SetLevel(CurrentLevel + 1);
}

void APDWeaponBase::SetLevel(int32 NewLevel)
{
    CurrentLevel = FMath::Clamp(NewLevel, 1, LevelStats.Num());
    CurrentAmmo = GetCurrentStats().MaxAmmo;
    OnWeaponLevelChanged.Broadcast(this, CurrentLevel);
}

void APDWeaponBase::SetDropped(bool bDropped)
{
    bIsDropped = bDropped;

    if (bDropped)
    {
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        WeaponMesh->SetSimulatePhysics(true);
    }
    else
    {
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetSimulatePhysics(false);
    }
}

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
    if (!HitActor->Implements<UPDDamageable>()) return;
    if (!IPDDamageable::Execute_IsAlive(HitActor)) return;

    FPDDamageInfo DamageInfo;
    DamageInfo.BaseDamage = DamageAmount;
    DamageInfo.Instigator = GetWeaponOwner();
    DamageInfo.DamageTypeClass = nullptr;

    IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
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

void APDWeaponBase::ToggleZoom()
{
    bIsZoomed = !bIsZoomed;

    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return;

    APlayerController* PC = Cast<APlayerController>(
        WeaponOwnerActor->GetInstigatorController());
    if (!PC) return;

    APlayerCameraManager* CamManager = PC->PlayerCameraManager;
    if (!CamManager) return;

    float TargetFOV = bIsZoomed ? ZoomedFOV : DefaultFOV;
    CamManager->SetFOV(TargetFOV);
}

void APDWeaponBase::EjectShell_Implementation()
{
    if (!ShellActorClass || !GetWorld()) return;

    FName SocketToUse = WeaponMesh->DoesSocketExist(EjectionPortSocket)
        ? EjectionPortSocket : MuzzleSocketName;

    FTransform SocketTM = WeaponMesh->GetSocketTransform(SocketToUse);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APDShellActor* Shell = GetWorld()->SpawnActor<APDShellActor>(
        ShellActorClass, SocketTM.GetLocation(), SocketTM.Rotator(), Params);

    if (Shell)
    {
        FVector WorldEjectDir = SocketTM.TransformVectorNoScale(ShellEjectLocalDir.GetSafeNormal());
        float SpeedWithVariance = ShellEjectSpeed * FMath::RandRange(0.9f, 1.1f);
        Shell->Launch(WorldEjectDir * SpeedWithVariance);
    }

    OnShellEjected.Broadcast(this);
}

void APDWeaponBase::DropMagazine_Implementation()
{
    if (!MagazineActorClass || !GetWorld()) return;

    if (MagazineMesh)
        MagazineMesh->SetVisibility(false);

    FTransform MagTM = MagazineMesh
        ? MagazineMesh->GetComponentTransform()
        : WeaponMesh->GetSocketTransform(MagazineSocketName);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APDMagazineActor* MagActor = GetWorld()->SpawnActor<APDMagazineActor>(
        MagazineActorClass, MagTM.GetLocation(), MagTM.Rotator(), Params);

    if (MagActor)
    {
        if (MagazineMesh && MagazineMesh->GetStaticMesh())
            MagActor->InitFromWeapon(
                MagazineMesh->GetStaticMesh(),
                MagazineMesh->GetMaterial(0));
        MagActor->Drop();
    }

    OnMagazineDropped.Broadcast(this);
}

void APDWeaponBase::AttachNewMagazine_Implementation()
{
    if (MagazineMesh)
        MagazineMesh->SetVisibility(true);

    OnMagazineAttached.Broadcast(this);
}

void APDWeaponBase::OnBoltPulled_Implementation()
{
    OnBoltPullEvent.Broadcast(this);
}

void APDWeaponBase::OnBoltReleased_Implementation()
{
    OnBoltReleaseEvent.Broadcast(this);
}

void APDWeaponBase::OnShellInserted_Implementation()
{
}

void APDWeaponBase::PlayWeaponMontage(UAnimMontage* Montage, FName StartSection)
{
    if (!Montage) return;
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    if (!AnimInst) return;

    AnimInst->Montage_Play(Montage);
    if (StartSection != NAME_None)
        AnimInst->Montage_JumpToSection(StartSection, Montage);
}

bool APDWeaponBase::IsPlayingMontage(UAnimMontage* Montage) const
{
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    return AnimInst ? AnimInst->Montage_IsPlaying(Montage) : false;
}

void APDWeaponBase::StopWeaponMontage(UAnimMontage* Montage)
{
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    if (AnimInst && Montage)
        AnimInst->Montage_Stop(0.15f, Montage);
}

void APDWeaponBase::BindMontageEndedForReload(UAnimMontage* Montage)
{
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    if (!AnimInst) return;

    FOnMontageEnded EndedDelegate;
    EndedDelegate.BindUObject(this, &APDWeaponBase::OnReloadMontageEnded);
    AnimInst->Montage_SetEndDelegate(EndedDelegate, Montage);
}

void APDWeaponBase::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (!bInterrupted)
        FinishReload();
    else
        bIsReloading = false;
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
