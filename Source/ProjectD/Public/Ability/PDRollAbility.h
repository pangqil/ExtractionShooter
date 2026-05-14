#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "PDRollAbility.generated.h"

UCLASS()
class PROJECTD_API UPDRollAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDRollAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="PD|Roll")
	TObjectPtr<UAnimMontage> RollMontage;

	// 구르기 이동 거리 (LaunchCharacter 세기)
	UPROPERTY(EditDefaultsOnly, Category="PD|Roll")
	float RollImpulseStrength = 800.f;

	// true: 입력 방향, false: 캐릭터 전방
	UPROPERTY(EditDefaultsOnly, Category="PD|Roll")
	bool bUseInputDirection = true;

private:
	UFUNCTION()
	void OnRollMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FVector GetRollDirection() const;
};
