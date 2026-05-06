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
    // 컴포넌트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Component")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
    TObjectPtr<USphereComponent> PickupCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Component")
    TObjectPtr<UStaticMeshComponent> MagazineMesh;

    // 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
    EWeaponType WeaponType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Config")
    FName MuzzleSocketName = TEXT("MuzzleSocket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Stats")
    TArray<FWeaponLevelStats> LevelStats;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;

    // 발사 후 볼트 액션 몽타주 (스나이퍼 전용)
    //   BoltPull(탄피배출) → EjectShell → BoltRelease
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TObjectPtr<UAnimMontage> BoltActionMontage;

    // ── NEW: 소켓 이름 (BP에서 무기 메시에 맞게 세팅) ──────────
    // 탄피가 튀어나오는 약실 소켓
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    FName EjectionPortSocket = TEXT("EjectionPort");

    // 탄창이 붙어있는 소켓 (MagazineMesh 위치 기준)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    FName MagazineSocketName = TEXT("MagazineSocket");

    // ── NEW: 스폰 클래스 (BP에서 총기 타입에 맞는 클래스 할당) ─
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TSubclassOf<APDShellActor> ShellActorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    TSubclassOf<APDMagazineActor> MagazineActorClass;

    // ── NEW: 탄피 배출 방향 오프셋 (로컬 기준) ─────────────────
    // 기본값: 우측 + 약간 위로 (AR 계열 표준 배출 방향)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    FVector ShellEjectLocalDir = FVector(0.f, 1.f, 0.3f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Animation")
    float ShellEjectSpeed = 200.f;


    // 런타임 상태
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
    // 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponFired OnWeaponFired;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponLevelChanged OnWeaponLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "PD|Weapon|Events")
    FOnWeaponReloaded OnWeaponReloaded;

    // BP에서 캐릭터 팔 애니메이션, 사운드 큐 등을 연결할 때 사용
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
    // 핵심 액션
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void Fire();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void Reload();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void OnEquip(AActor* NewOwner);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void OnUnequip();

    // 각 AnimNotify에서 직접 호출 — BP에서 추가 효과 오버라이드 가능
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

    // 샷건: 탄 한 발 약실에 넣기 완료
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnShellInserted();

    // IPDInteractable 구현
    // 플레이어가 F키 누르면 줍기
    virtual void Interact_Implementation(AActor* Interactor) override;

    // 레벨 시스템
    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void UpgradeLevel();

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void SetLevel(int32 NewLevel);

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
    void SetDropped(bool bDropped);

    // Getter
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
    

    // WeaponMesh 의 AnimInstance 에 몽타주 재생
    void PlayWeaponMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

    // 몽타주 재생 중 여부 확인
    bool IsPlayingMontage(UAnimMontage* Montage) const;

    // 몽타주 강제 중단 (e.g. 샷건 재장전 도중 발사)
    void StopWeaponMontage(UAnimMontage* Montage);

    // 몽타주 종료 콜백 바인딩 — FinishReload 자동 호출 용도
    void BindMontageEndedForReload(UAnimMontage* Montage);

private:
    // 내부 콜백: 재장전 몽타주 종료 시 FinishReload 호출
    UFUNCTION()
    void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};


   

   