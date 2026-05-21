#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "GameplayEffectTypes.h"
#include "PDSprintAbility.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTD_API UPDSprintAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDSprintAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Sprint")
	TSubclassOf<UGameplayEffect> SprintEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Sprint")
	TSubclassOf<UGameplayEffect> SprintCostEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Sprint", meta=(ClampMin="0.0"))
	float MinStaminaToStart = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Sprint", meta=(ClampMin="0.0"))
	float MinStaminaToKeepSprinting = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Sprint")
	bool bEndWhenStaminaEmpty = true;

private:
	FActiveGameplayEffectHandle SprintEffectHandle;
	FActiveGameplayEffectHandle SprintCostEffectHandle;
	FDelegateHandle StaminaChangedDelegateHandle;

	FActiveGameplayEffectHandle ApplySprintGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass,
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const;
	void RemoveSprintGameplayEffects(UAbilitySystemComponent* ASC) const;
	void SyncCharacterMovementSpeed() const;
	void BindStaminaChanged();
	void UnbindStaminaChanged();
	void OnStaminaChanged(const FOnAttributeChangeData& Data);
};
