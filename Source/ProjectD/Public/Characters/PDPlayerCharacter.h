#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Type/Types.h"
#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDRifle.h"
#include "PDPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSwapped, APDWeaponBase*, NewWeapon, EWeaponSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPickedUp, APDWeaponBase*, Weapon);

UCLASS(abstract)
class APDPlayerCharacter : public APDCharacterBase
{
	GENERATED_BODY()

public:
	APDPlayerCharacter();

	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

protected:
	// 오버라이드
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void HandleDeath(AActor* Killer) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

protected:
	// 무기 슬롯
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Player|Weapon")
	TArray<TObjectPtr<APDWeaponBase>> WeaponSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Player|Weapon")
	EWeaponSlot CurrentSlot = EWeaponSlot::None;

public:
	// 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "PD|Player|Events")
	FOnWeaponSwapped OnWeaponSwapped;

	UPROPERTY(BlueprintAssignable, Category = "PD|Player|Events")
	FOnWeaponPickedUp OnWeaponPickedUp;

public:
	// 무기 관리
	UFUNCTION(BlueprintCallable, Category = "PD|Player|Weapon")
	void PickupWeapon(APDWeaponBase* Weapon);

	UFUNCTION(BlueprintCallable, Category = "PD|Player|Weapon")
	void SwitchToSlot(EWeaponSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "PD|Player|Weapon")
	void DropCurrentWeapon();

	// Getter
	UFUNCTION(BlueprintPure, Category = "PD|Player|Weapon")
	APDWeaponBase* GetCurrentWeapon() const;

	UFUNCTION(BlueprintPure, Category = "PD|Player|Weapon")
	APDWeaponBase* GetWeaponInSlot(EWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "PD|Player")
	FORCEINLINE EWeaponSlot GetCurrentSlot() const { return CurrentSlot; }

protected:	
	EWeaponSlot GetSlotForWeaponType(EWeaponType Type) const;
};
