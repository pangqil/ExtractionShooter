#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Interfaces/PDSurvivalSource.h"
#include "PDPlayerCharacter.generated.h"

class UPDVisionComponent;
class UCameraComponent;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSwapped, APDWeaponBase*, NewWeapon, EWeaponSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPickedUp, APDWeaponBase*, Weapon);

UCLASS(abstract)
class APDPlayerCharacter : public APDCharacterBase,
						   public IPDSurvivalSource
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDVisionComponent> VisionComponent;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Survival")
	TSubclassOf<UGameplayEffect> HungerDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Survival")
	TSubclassOf<UGameplayEffect> ThirstDecayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Survival")
	TSubclassOf<UGameplayEffect> StarvingEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Survival")
	TSubclassOf<UGameplayEffect> DehydratedEffectClass;

	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	
public:
	APDPlayerCharacter();
	virtual void InitAbilitySystem() override;
	

	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

	// IPDSurvivalSource
	virtual TSubclassOf<UGameplayEffect> GetHungerDecayEffectClass()  const override { return HungerDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetThirstDecayEffectClass()  const override { return ThirstDecayEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetStarvingEffectClass()     const override { return StarvingEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetDehydratedEffectClass()   const override { return DehydratedEffectClass; }
};
