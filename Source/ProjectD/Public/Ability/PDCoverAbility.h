#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "PDCoverAbility.generated.h"

class UPDCoverComponent;

UCLASS()
class PROJECTD_API UPDCoverAbility : public UPDGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UPDCoverAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UPDCoverComponent* GetCoverComponent() const;
};
