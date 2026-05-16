#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Camera/CameraShakeBase.h"
#include "PDRangedWeaponBase.generated.h"

class UPDInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, APDWeaponBase*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded, APDWeaponBase*, Weapon);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDRangedWeaponBase : public APDWeaponBase
{
	GENERATED_BODY()

public:
	APDRangedWeaponBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Config")
	FName MuzzleSocketName = TEXT("MuzzleSocket");

	/** true면 발사 버튼을 누르는 동안 자동으로 연사. Rifle 생성자에서 true로 설정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Config")
	bool bFullAuto = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Effects")
	TObjectPtr<UParticleSystem> TracerEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Effects")
	TObjectPtr<UParticleSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Effects")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Ammo")
	FName AmmoItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Recoil")
	TSubclassOf<UCameraShakeBase> FireCameraShakeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Recoil", meta=(ClampMin="0.0"))
	float RecoilSpreadPerShot = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Recoil", meta=(ClampMin="0.0"))
	float MaxRecoilSpread = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Recoil", meta=(ClampMin="0.0"))
	float RecoilRecoveryRate = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|Recoil")
	float CurrentRecoilSpread = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|State")
	int32 CurrentAmmo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|State")
	bool bIsReloading = false;

	FTimerHandle SpreadRecoveryHandle;

	UFUNCTION()
	void TickSpreadRecovery();

public:
	bool bCanFire = true;
	FTimerHandle FireCooldownHandle;
	FTimerHandle ReloadHandle;

	UPROPERTY(BlueprintAssignable, Category="PD|Weapon|Events")
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category="PD|Weapon|Events")
	FOnWeaponReloaded OnWeaponReloaded;

	virtual void Fire_Implementation() override;
	virtual void Reload_Implementation() override;
	virtual void OnEquip_Implementation(AActor* NewOwner) override;
	virtual void OnUnequip_Implementation() override;

	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Recoil")
	float GetCurrentRecoilSpread() const { return CurrentRecoilSpread; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Ammo")
	int32 GetAvailableAmmoCount() const;

	void FinishReload();

public:
	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Config")
	bool IsFullAuto() const { return bFullAuto; }

	// GA_Reload 취소 시 호출 — 리로드 타이머/몽타주/플래그 초기화
	void CancelReload();

protected:
	bool HasAmmoToReload() const;
	void ApplyDamage(AActor* HitActor, float DamageAmount);
	void PostFire();
	void ResetFireCooldown();
	void SpawnTracerEffect(const FVector& Start, const FVector& End);
	void PlayWeaponMontage(UAnimMontage* Montage, FName StartSection = NAME_None);
	bool IsPlayingMontage(UAnimMontage* Montage) const;
	void StopWeaponMontage(UAnimMontage* Montage);
	void BindMontageEndedForReload(UAnimMontage* Montage);
	void PlayFireEffects();
	void ApplyRecoil();
	
	APlayerController* GetOwnerPlayerController() const;
	UPDInventoryComponent* GetOwnerInventory() const;

private:
	UFUNCTION()
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
