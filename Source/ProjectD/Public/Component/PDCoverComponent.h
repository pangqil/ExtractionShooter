// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Cover/PDCoverBase.h"
#include "PDCoverComponent.generated.h"

class APDCoverBase;
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

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover")
	float CoverExitDistance=250.f;
	
private:
	void OnCoverArrived();
	void StartCoverProximityCheck();
	void ApplyCoverState();
	void RemoveCoverState();

	UAbilitySystemComponent* GetASC() const;
	ACharacter* GetOwnerCharacter() const;

	TWeakObjectPtr<APDCoverBase> CurrentCoverActor;
	FTimerHandle CoverArrivalCheckHandle;
	FTimerHandle CoverProximityCheckHandle;
};
