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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void OnUnequip();

	// AnimNotify 콜백
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon|Animation")
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
};
