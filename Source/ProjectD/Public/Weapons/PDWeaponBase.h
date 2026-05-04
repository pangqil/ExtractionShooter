#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "Interfaces/PDInteractable.h"
#include "PDWeaponBase.generated.h"

class APDWeaponBase;
class USphereComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponLevelChanged, APDWeaponBase*, Weapon, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded, APDWeaponBase*, Weapon);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDWeaponBase();

protected:

    virtual void BeginPlay() override;

    // 발사 가능 여부
    bool CanFire() const;

    // 데미지 적용
    void ApplyDamage(AActor* HitActor, float DamageAmount);

    // 발사 쿨다운 리셋
    void ResetFireCooldown();

    // 재장전 완료
    void FinishReload();

    // 발사 후처리 (탄약 감소, 쿨다운, 델리게이트)
    void PostFire();

protected:
    // 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
    TObjectPtr<USphereComponent> PickupCollision;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|FX")
    TObjectPtr<UNiagaraSystem> MuzzleFlashFX;  // 총구 이펙트

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|FX")
    TObjectPtr<UNiagaraSystem> BulletTrailFX;  // 총알 궤적

    // 총기 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
    EWeaponType WeaponType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
    FName MuzzleSocketName = TEXT("MuzzleSocket");

    // 레벨별 스탯 배열 (인덱스 0=Lv1, 1=Lv2, 2=Lv3)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Stats")
    TArray<FWeaponLevelStats> LevelStats;

    // 런타임 상태
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
    int32 CurrentLevel = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
    int32 CurrentAmmo = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|State")
    bool bIsDropped = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|State")
    bool bIsReloading = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon")
    TWeakObjectPtr<AActor> WeaponOwner;

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void SetDropped(bool bDropped);

    bool bCanFire = true;

    FTimerHandle FireCooldownHandle;
    FTimerHandle ReloadHandle;

public:
    // 이벤트 발생
    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponFired OnWeaponFired;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponLevelChanged OnWeaponLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponReloaded OnWeaponReloaded;

public:
    // 핵심 액션
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void Fire();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnEquip(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnUnequip();

    // 레벨 시스템
    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void UpgradeLevel();

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void SetLevel(int32 NewLevel);

    // Getter
    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    const FWeaponLevelStats& GetCurrentStats() const;

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE int32 GetCurrentLevel()    const { return CurrentLevel; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE int32 GetCurrentAmmo()     const { return CurrentAmmo; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE bool IsReloading()         const { return bIsReloading; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE AActor* GetWeaponOwner()   const { return WeaponOwner.Get(); }

    UFUNCTION(BlueprintPure, Category = "PD|Weapon")
    FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

    virtual void Interact_Implementation(AActor* Interactor) override;

protected:
    UFUNCTION()
    void OnPickupOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);
};


   

   