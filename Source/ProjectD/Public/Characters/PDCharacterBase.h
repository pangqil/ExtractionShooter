#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PDDamageable.h"
#include "Interfaces/PDInteractable.h"
#include "AbilitySystemInterface.h"
#include "PDCharacterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, Killer);
class UGameplayEffect;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDCharacterBase : public ACharacter, 
										public IPDDamageable, 
										public IPDInteractable, 
										public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APDCharacterBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

public:
	UPROPERTY(BlueprintAssignable, Category = "PD|Damage")
	FOnDeathSignature OnDeathDelegate;

	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual float GetMaxHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;
	virtual void Interact_Implementation(AActor* Interactor) override {}
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override {return nullptr;}
	
	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void AttachActorToWeaponSocket(AActor* ActorToAttach);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnDeath(AActor* Killer);

	virtual void HandleDeath(AActor* Killer);
protected:

	
	virtual void BeginPlay() override;
};
