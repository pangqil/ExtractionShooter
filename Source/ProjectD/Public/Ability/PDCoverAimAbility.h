#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "PDCoverAimAbility.generated.h"

UCLASS()
class PROJECTD_API UPDCoverAimAbility : public UPDGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UPDCoverAimAbility();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

    UFUNCTION(BlueprintImplementableEvent, Category="PD|Cover")
    void BP_OnStartAim(FVector AimPosition, FRotator AimRotation);

    UFUNCTION(BlueprintImplementableEvent, Category="PD|Cover")
    void BP_OnReturnToCover(FVector CoverPosition, FRotator CoverRotation);

protected:
    UPROPERTY(EditDefaultsOnly, Category="PD|Cover")
    float PeekOffset = 60.f;
};
