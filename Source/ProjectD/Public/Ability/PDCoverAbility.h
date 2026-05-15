#pragma once

#include "CoreMinimal.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "PDCoverAbility.generated.h"

class APDCoverBase;

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

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

    UFUNCTION(BlueprintImplementableEvent, Category="PD|Cover")
    void BP_OnEnterCover(APDCoverBase* CoverActor, FVector SnapLocation, FRotator SnapRotation);

    UFUNCTION(BlueprintImplementableEvent, Category="PD|Cover")
    void BP_OnExitCover();

    // BP에서 보간 완료 후 호출 -> Cover_Active 태그 부여
    UFUNCTION(BlueprintCallable, Category="PD|Cover")
    void FinishEnterCover();

    // 이동키 등 외부 종료 요청
    UFUNCTION(BlueprintCallable, Category="PD|Cover")
    void NotifyExitCover();

protected:
    UPROPERTY(EditDefaultsOnly, Category="PD|Cover")
    TSubclassOf<UGameplayEffect> CoverBuffGE;

private:
    void ApplyCoverBuff();
    void RemoveCoverBuff();

    TWeakObjectPtr<APDCoverBase> CurrentCover;
    FActiveGameplayEffectHandle CoverBuffHandle;
};
