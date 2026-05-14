#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "Cover/PDCoverBase.h"
#include "PDCoverComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDCoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDCoverComponent();

	UFUNCTION(BlueprintCallable, Category="PD|Cover")
	void TryEnterCover();

	UFUNCTION(BlueprintCallable, Category="PD|Cover")
	void ExitCover();

	UFUNCTION(BlueprintPure, Category="PD|Cover")
	bool IsInCover() const {return CurrentCoverActor.IsValid();}

	void ForceExitCover();

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover")
	float CoverSearchRadius=300.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover")
	float CoverArrivalTolerance=40.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover|GAS")
	TSubclassOf<UGameplayEffect> CoverBuff;

private:
	void OnCoverArrived();
	void ApplyCoverState();
	void RemoveCoverState();
	void LockMovement();
	void UnlockMovement();

	UAbilitySystemComponent* GetASC() const;
	ACharacter* GetOwnerCharacter() const;

	TWeakObjectPtr<APDCoverBase> CurrentCoverActor;
	FVector SnapLocation=FVector::ZeroVector;
	FRotator SnapRotation=FRotator::ZeroRotator;

	FTimerHandle CoverArrivalCheckHandle;
	FActiveGameplayEffectHandle ActiveCoverBuffHandle;
};
