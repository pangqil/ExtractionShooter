#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "Interfaces/PDInteractable.h"
#include "PDWeaponBase.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;

/**
 * 모든 무기의 추상 베이스.
 *
 * 책임:
 *  - 레벨별 스탯(FWeaponLevelStats) 관리
 *  - 발사/재장전/착탈 공통 흐름 (CanFire, PostFire, FinishReload)
 *  - AnimNotify 콜백 BlueprintNativeEvent (EjectShell, MagOut/In, BoltPull/Release, InsertShell)
 *  - IPDInteractable: 플레이어가 F키로 줍기
 *
 * Senior: 구체적인 발사 판정·데미지는 자식에서 오버라이드.
 *         스탯은 LevelStats 배열을 자식 생성자에서 Add({...})로 채울 것.
 */
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
	/** 무기 메시. 애니메이션·소켓 위치 모두 이 컴포넌트 기준 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon")
	TWeakObjectPtr<AActor> WeaponOwner;
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

	/** 자식 생성자에서 설정 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon")
	EWeaponType WeaponType = EWeaponType::None;

	/** 레벨 1부터 순서대로. 자식 생성자에서 Add({...}) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Stats")
	TArray<FWeaponLevelStats> LevelStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|Stats")
	int32 CurrentLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|Ammo")
	int32 CurrentAmmo = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Config")
	FName MuzzleSocketName = TEXT("MuzzleSocket");

	/** 라이플·스나이퍼 재장전 몽타주 (샷건은 자체 로직) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Config")
	float DefaultFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Config")
	float ZoomedFOV = 70.f;

	bool bIsReloading = false;
	FTimerHandle ReloadHandle;

	// ─── 공통 헬퍼 ────────────────────────────────────────────

	/** 현재 레벨 스탯 참조. LevelStats 비어있으면 static default 반환 */
	const FWeaponLevelStats& GetCurrentStats() const;

	/** 발사 가능 여부 (탄 있음 + 재장전 중 아님) */
	bool CanFire() const;

	/** 발사 후 공통 처리 (CurrentAmmo 감소) */
	void PostFire();

	/** IPDDamageable 인터페이스를 통해 데미지 적용 */
	void ApplyDamage(AActor* Target, float DamageAmount);

	/** 재장전 완료 — 탄 채우기 + 플래그 해제 */
	void FinishReload();

	void PlayWeaponMontage(UAnimMontage* Montage);
	void StopWeaponMontage(UAnimMontage* Montage);
	/** ReloadMontage 종료 시 FinishReload() 자동 호출 바인딩 */
	void BindMontageEndedForReload(UAnimMontage* Montage);

public:
	// ─── BlueprintNativeEvent UFUNCTIONs ──────────────────────

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void OnEquip(AActor* NewOwner);
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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void OnUnequip();

	// AnimNotify 콜백
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
	void EjectShell();
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void EjectShell();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
	void DropMagazine();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
	void AttachNewMagazine();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
	void OnBoltPulled();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
	void OnBoltReleased();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
	void OnShellInserted();

	// IPDInteractable — 플레이어가 F키로 줍기
	virtual void Interact_Implementation(AActor* Interactor) override;

	// ─── Getter ───────────────────────────────────────────────

	FORCEINLINE AActor* GetWeaponOwner()  const { return WeaponOwner.Get(); }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetCurrentAmmo()    const { return CurrentAmmo; }
	FORCEINLINE int32 GetMaxAmmo()        const
	{
		return LevelStats.IsValidIndex(CurrentLevel - 1)
			? LevelStats[CurrentLevel - 1].MaxAmmo : 0;
	}
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
