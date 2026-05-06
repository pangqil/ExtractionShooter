#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "Animation/AnimMontage.h"
#include "Interfaces/PDInteractable.h"
#include "PDWeaponBase.generated.h"

class APDWeaponBase;
class APDShellActor;
class APDMagazineActor;
class USphereComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponLevelChanged, APDWeaponBase*, Weapon, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoomToggled, bool, bIsZoomed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAnimEvent, APDWeaponBase*, Weapon);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDWeaponBase();

protected:
    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Component")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
    TObjectPtr<USphereComponent> PickupCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
    TObjectPtr<UStaticMeshComponent> MagazineMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
    EWeaponType WeaponType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
    FName MuzzleSocketName = TEXT("MuzzleSocket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Stats")
    TArray<FWeaponLevelStats> LevelStats;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TObjectPtr<UAnimMontage> BoltActionMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    FName EjectionPortSocket = TEXT("EjectionPort");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    FName MagazineSocketName = TEXT("MagazineSocket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TSubclassOf<APDShellActor> ShellActorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TSubclassOf<APDMagazineActor> MagazineActorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    FVector ShellEjectLocalDir = FVector(0.f, 1.f, 0.3f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    float ShellEjectSpeed = 200.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
    int32 CurrentLevel = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
    int32 CurrentAmmo = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
    bool bIsReloading = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|State")
    bool bIsDropped = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon")
    TWeakObjectPtr<AActor> WeaponOwner;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Zoom")
    float DefaultFOV = 90.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Zoom")
    float ZoomedFOV = 60.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Zoom")
    float ZoomInterpSpeed = 10.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Zoom")
    bool bIsZoomed = false;

public:
    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void ToggleZoom();

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE bool IsZoomed() const { return bIsZoomed; }

    bool bCanFire = true;
    FTimerHandle FireCooldownHandle;
    FTimerHandle ReloadHandle;

public:
    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponFired OnWeaponFired;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponLevelChanged OnWeaponLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponReloaded OnWeaponReloaded;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|AnimEvents")
    FOnWeaponAnimEvent OnShellEjected;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|AnimEvents")
    FOnWeaponAnimEvent OnMagazineDropped;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|AnimEvents")
    FOnWeaponAnimEvent OnMagazineAttached;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|AnimEvents")
    FOnWeaponAnimEvent OnBoltPullEvent;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|AnimEvents")
    FOnWeaponAnimEvent OnBoltReleaseEvent;

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void Fire();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void Reload();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void OnEquip(AActor* NewOwner);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void OnUnequip();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void EjectShell();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void DropMagazine();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void AttachNewMagazine();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnBoltPulled();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnBoltReleased();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnShellInserted();

    virtual void Interact_Implementation(AActor* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void UpgradeLevel();

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void SetLevel(int32 NewLevel);

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void SetDropped(bool bDropped);

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    const FWeaponLevelStats& GetCurrentStats() const;

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE int32 GetCurrentLevel()     const { return CurrentLevel; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE int32 GetCurrentAmmo()      const { return CurrentAmmo; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE bool IsReloading()          const { return bIsReloading; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE AActor* GetWeaponOwner()    const { return WeaponOwner.Get(); }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon|Zoom")
    FORCEINLINE bool GetIsZoomed() const { return bIsZoomed; }

    void FinishReload();

protected:
    bool CanFire() const;
    void ApplyDamage(AActor* HitActor, float DamageAmount);
    void PostFire();
    void ResetFireCooldown();

    void PlayWeaponMontage(UAnimMontage* Montage, FName StartSection = NAME_None);
    bool IsPlayingMontage(UAnimMontage* Montage) const;
    void StopWeaponMontage(UAnimMontage* Montage);
    void BindMontageEndedForReload(UAnimMontage* Montage);

private:
    UFUNCTION()
    void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
