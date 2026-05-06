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
    // 이미 장착된 무기면 무시
    if (WeaponOwner.IsValid()) return;

    APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
    if (!Player) return;

    Player->PickupWeapon(this);
    SetDropped(false);
}

// 핵심 액션

void APDWeaponBase::Fire_Implementation() {}

void APDWeaponBase::Reload_Implementation()
{
    if (bIsReloading) return;
    if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;

    bIsReloading = true;

    if (ReloadMontage)
    {
        // 몽타주가 설정된 경우 FinishReload
        PlayWeaponMontage(ReloadMontage);
        BindMontageEndedForReload(ReloadMontage);
    }
    else
    {
        // 몽타주 없을 때 폴백: 기존 타이머 방식
        GetWorldTimerManager().SetTimer(
            ReloadHandle, this,
            &APDWeaponBase::FinishReload,
            GetCurrentStats().ReloadTime, false);
    }
}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
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

// 레벨 시스템

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
    
    if (!HitActor->Implements<UPDDamageable>()) return;
    // 이미 죽은 대상에게 데미지 방지
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

    // 카메라 FOV 변경
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
    UE_LOG(LogTemp, Warning, TEXT("EjectShell called!"));

    if (!ShellActorClass || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("ShellActorClass 없음!"));
        return;
    }

    if (!ShellActorClass || !GetWorld()) return;

    // 소켓 없으면 머즐 소켓으로 폴백
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
        // 로컬 배출 방향 월드 방향으로 변환 후 속도 부여
        FVector WorldEjectDir = SocketTM.TransformVectorNoScale(ShellEjectLocalDir.GetSafeNormal());
        // 약간의 랜덤성 추가 
        float SpeedWithVariance = ShellEjectSpeed * FMath::RandRange(0.9f, 1.1f);
        Shell->Launch(WorldEjectDir * SpeedWithVariance);
    }

    OnShellEjected.Broadcast(this);
}

void APDWeaponBase::DropMagazine_Implementation()
{
    if (!MagazineActorClass || !GetWorld()) return;

    // 1. 무기에 붙은 탄창 메시 숨기기
    if (MagazineMesh)
        MagazineMesh->SetVisibility(false);

    // 2. 같은 위치·회전으로 MagazineActor 스폰
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
        // 무기 탄창과 같은 메시/머티리얼을 동적으로 복사
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
    // 새 탄창 삽입 완료  탄창 메시 다시 표시
    if (MagazineMesh)
        MagazineMesh->SetVisibility(true);

    OnMagazineAttached.Broadcast(this);
}

void APDWeaponBase::OnBoltPulled_Implementation()
{
    // 기본: 델리게이트만 방송 (사운드/이펙트는 BP에서 연결)
    OnBoltPullEvent.Broadcast(this);
}

void APDWeaponBase::OnBoltReleased_Implementation()
{
    OnBoltReleaseEvent.Broadcast(this);
}

void APDWeaponBase::OnShellInserted_Implementation()
{
    // 기본 구현은 비어있음  PDShotgun 에서 오버라이드
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
    // 중단되지 않은 경우에만 장전 완료
    if (!bInterrupted)
        FinishReload();
    else
        bIsReloading = false;
}