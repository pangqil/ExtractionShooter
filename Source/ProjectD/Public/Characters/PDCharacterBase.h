#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PDDamageable.h"
#include "Interfaces/PDInteractable.h"
#include "PDCharacterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, Killer);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDCharacterBase : public ACharacter, public IPDDamageable, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDCharacterBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stat")
	float CurrentHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");

public:
	UPROPERTY(BlueprintAssignable, Category = "PD|Damage")
	FOnDeathSignature OnDeathDelegate;

	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;
	virtual float GetCurrentHealth_Implementation() const override { return CurrentHealth; }
	virtual float GetMaxHealth_Implementation() const override { return MaxHealth; }
	virtual bool IsAlive_Implementation() const override { return CurrentHealth > 0.f; }

	virtual void Interact_Implementation(AActor* Interactor) override {}

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void AttachActorToWeaponSocket(AActor* ActorToAttach);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnDeath(AActor* Killer);

	virtual void HandleDeath(AActor* Killer);
	virtual void BeginPlay() override;
};
