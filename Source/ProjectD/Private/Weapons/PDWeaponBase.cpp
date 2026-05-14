#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDShellActor.h"
#include "Weapons/PDMagazineActor.h"
#include "Characters/PDPlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Component/PDWeaponComponent.h"

#include "Animation/AnimInstance.h"
#include "Interfaces/PDDamageable.h"
#include "Items/PDInventoryComponent.h"
#include "Engine/DataTable.h"

namespace
{
	const FPDItemData* FindItemDataByID(const UDataTable* DataTable, const FName& ItemID)
	{
		if (!DataTable || ItemID.IsNone())
		{
			return nullptr;
		}

		static const FString Context(TEXT("FindItemDataByID"));
		TArray<FPDItemData*> Rows;
		DataTable->GetAllRows<FPDItemData>(Context, Rows);

		for (const FPDItemData* Row : Rows)
		{
			if (Row && Row->ItemID == ItemID)
			{
				return Row;
			}
		}

		return nullptr;
	}
}

APDWeaponBase::APDWeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetupAttachment(RootComponent);
    PickupCollision->SetSphereRadius(80.f);

    // Interact 트레이스(ECC_Visibility)만 Block — PDItemBase와 동일 패턴
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
    PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    PickupCollision->SetGenerateOverlapEvents(false);

    MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
    MagazineMesh->SetupAttachment(WeaponMesh, MagazineSocketName);
    MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void APDWeaponBase::BeginPlay()
{
    Super::BeginPlay();
    if (LevelStats.IsValidIndex(0))
        CurrentAmmo = LevelStats[0].MaxAmmo;

    LoadItemData();
}

void APDWeaponBase::LoadItemData()
{
    if (!ItemDataTable || ItemRowName.IsNone()) return;

    const FPDItemData* Row = FindItemDataByID(ItemDataTable, ItemRowName);
    if (!Row) return;

    CachedItemData = *Row;
    if (CachedItemData.ItemID.IsNone())
    {
        CachedItemData.ItemID = ItemRowName;
    }
}

void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
    // 월드 무기는 상호작용 시 즉시 캐릭터에 장착하지 않고, 항상 인벤토리에 먼저 들어간다.
    // 실제 장착/메시 부착은 인벤토리 우클릭 > 장착하기에서 EquipmentComponent를 통해 처리한다.
    if (WeaponOwner.IsValid()) return;

    APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
    if (!Player) return;

    if (CachedItemData.ItemID.IsNone())
    {
        OnPickupFailed();
        return;
    }

    UPDInventoryComponent* Inventory = Player->FindComponentByClass<UPDInventoryComponent>();
    if (!Inventory || !Inventory->AddItem(CachedItemData, 1))
    {
        OnPickupFailed();
        return;
    }

    Destroy();
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
    SetOwner(NewOwner);
    WeaponMesh->SetSimulatePhysics(false);
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (CurrentAmmo == 0 && !LevelStats.IsEmpty())
        CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDWeaponBase::OnUnequip_Implementation()
{
    WeaponOwner = nullptr;
    GetWorldTimerManager().ClearTimer(FireCooldownHandle);
    GetWorldTimerManager().ClearTimer(ReloadHandle);
    bIsReloading = false;
    bCanFire = true;
}

void APDWeaponBase::EjectShell_Implementation()
{
    if (!ShellActorClass || !GetWorld()) return;

    FName Socket = WeaponMesh->DoesSocketExist(EjectionPortSocket)
        ? EjectionPortSocket : MuzzleSocketName;

    FTransform TM = WeaponMesh->GetSocketTransform(Socket);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (APDShellActor* Shell = GetWorld()->SpawnActor<APDShellActor>(
        ShellActorClass, TM.GetLocation(), TM.Rotator(), Params))
    {
        FVector Dir = TM.TransformVectorNoScale(ShellEjectLocalDir.GetSafeNormal());
        Shell->Launch(Dir * ShellEjectSpeed * FMath::RandRange(0.9f, 1.1f));
    }
}

void APDWeaponBase::DropMagazine_Implementation()
{
    if (!MagazineActorClass || !GetWorld()) return;

    if (MagazineMesh) MagazineMesh->SetVisibility(false);

    FTransform MagTM = MagazineMesh
        ? MagazineMesh->GetComponentTransform()
        : WeaponMesh->GetSocketTransform(MagazineSocketName);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (APDMagazineActor* Mag = GetWorld()->SpawnActor<APDMagazineActor>(
        MagazineActorClass, MagTM.GetLocation(), MagTM.Rotator(), Params))
    {
        if (MagazineMesh && MagazineMesh->GetStaticMesh())
            Mag->InitFromWeapon(MagazineMesh->GetStaticMesh(), MagazineMesh->GetMaterial(0));
        Mag->Drop();
    }
}

void APDWeaponBase::AttachNewMagazine_Implementation()
{
    if (MagazineMesh) MagazineMesh->SetVisibility(true);
}

// 볼트/슬러그 — BP 오버라이드용 빈 구현
void APDWeaponBase::OnBoltPulled_Implementation() {}
void APDWeaponBase::OnBoltReleased_Implementation() {}
void APDWeaponBase::OnShellInserted_Implementation() {}

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
    static FWeaponLevelStats DefaultStats;
    if (LevelStats.IsEmpty()) return DefaultStats;
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
    if (IPDDamageable::Execute_GetCurrentHealth(HitActor) <= 0.f) return;

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

    ApplyRecoil();

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

// 반동로직
void APDWeaponBase::ApplyRecoil()
{
    // 1. 카메라 쉐이크
    if (FireCameraShakeClass)
    {
        if (APlayerController* PC = GetOwnerPlayerController())
            PC->PlayerCameraManager->StartCameraShake(FireCameraShakeClass);
    }

    // 2. 스프레드 누적
    CurrentRecoilSpread = FMath::Min(
        CurrentRecoilSpread + RecoilSpreadPerShot,
        MaxRecoilSpread);

    // 스프레드 회복 타이머 (재시작)
    GetWorldTimerManager().SetTimer(
        SpreadRecoveryHandle, this,
        &APDWeaponBase::TickSpreadRecovery,
        0.016f, true);  // ~60fps

   
    if (WeaponMesh)
    {
        // 원본 회전 저장 (처음 발사 시)
        if (!GetWorldTimerManager().IsTimerActive(MeshRecoilRecoveryHandle))
            OriginalMeshRelRotation = WeaponMesh->GetRelativeRotation();

        
        FRotator KickedRot = WeaponMesh->GetRelativeRotation() + MeshRecoilKick;
        WeaponMesh->SetRelativeRotation(KickedRot);

        // 메시 복구 타이머
        GetWorldTimerManager().SetTimer(
            MeshRecoilRecoveryHandle, this,
            &APDWeaponBase::TickMeshRecoilRecovery,
            0.016f, true);
    }
}

void APDWeaponBase::TickSpreadRecovery()
{
    CurrentRecoilSpread -= RecoilRecoveryRate * 0.016f;

    if (CurrentRecoilSpread <= 0.f)
    {
        CurrentRecoilSpread = 0.f;
        GetWorldTimerManager().ClearTimer(SpreadRecoveryHandle);
    }
}
// 메시 반동 복구
void APDWeaponBase::TickMeshRecoilRecovery()
{
    if (!WeaponMesh) return;

    FRotator Current = WeaponMesh->GetRelativeRotation();
    FRotator Target = OriginalMeshRelRotation;

    FRotator NewRot = FMath::RInterpTo(Current, Target,
        0.016f, MeshRecoilRecoverySpeed);

    WeaponMesh->SetRelativeRotation(NewRot);

    // 거의 다 돌아왔으면 타이머 종료
    if (Current.Equals(Target, 0.1f))
    {
        WeaponMesh->SetRelativeRotation(Target);
        GetWorldTimerManager().ClearTimer(MeshRecoilRecoveryHandle);
    }
}

APlayerController* APDWeaponBase::GetOwnerPlayerController() const
{
    if (!WeaponOwner.IsValid()) return nullptr;
    return Cast<APlayerController>(WeaponOwner->GetInstigatorController());
}

FVector APDWeaponBase::GetAimDirectionFromOwner(const FVector& StartLocation) const
{
    if (!WeaponOwner.IsValid()) return FVector::ForwardVector;

    // WeaponComponent에 조준 방향 위임 (플레이어/적 구분은 컴포넌트가 처리)
    if (UPDWeaponComponent* Comp =
        WeaponOwner->FindComponentByClass<UPDWeaponComponent>())
    {
        return Comp->GetAimDirection(StartLocation);
    }

    return WeaponOwner->GetActorForwardVector();
}