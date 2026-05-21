#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "PDReviveAbility.generated.h"

class APDCharacterBase;
class UAnimInstance;
class UAnimMontage;

UCLASS()
class PROJECTD_API UPDReviveAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDReviveAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive")
	TObjectPtr<UAnimMontage> ReviveLoopMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive")
	TObjectPtr<UAnimMontage> ReviveCompleteMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Revive", meta=(ClampMin="0.0", ForceUnits="s"))
	float ReviveCompleteDuration = 0.35f;

private:
	UFUNCTION()
	void OnReviveDelayFinished();

	UFUNCTION()
	void OnReviveCompleteDelayFinished();

	UFUNCTION()
	void OnReviveCancelEvent(FGameplayEventData Payload);

	UAnimInstance* GetReviverAnimInstance() const;
	void PlayReviveLoopMontage();
	float PlayReviveCompleteMontage();
	void StopReviveMontages(float BlendOutTime = 0.15f);

	UPROPERTY()
	TObjectPtr<APDCharacterBase> ReviveTarget;

	bool bReceivedCancelEvent = false;
};
