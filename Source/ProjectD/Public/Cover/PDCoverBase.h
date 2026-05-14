// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "PDCoverBase.generated.h"

UENUM(BlueprintType)
enum class ECoverState:uint8
{
	Normal,
	Damaged,
	Destroyed
};

USTRUCT(BlueprintType)
struct FCoverSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LocalOffset=FVector::ZeroVector;

	UPROPERTY(EditAnywhere)
	FRotator FacingRotation=FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere)
	bool bOccupied=false;

	UPROPERTY(VisibleAnywhere)
	TWeakObjectPtr<AActor> Occupant;
};

UCLASS()
class PROJECTD_API APDCoverBase : public AActor
{
	GENERATED_BODY()
public:
	APDCoverBase();
	
	FCoverSlot* FindBestSlot(AActor* Requster);
	
	bool OccupySlot(AActor* Requester, FCoverSlot* Slot);
	void ReleaseSlot(AActor* Requester);
	
	void TakeCoverDamage(float Damage);
	
	UFUNCTION(BlueprintPure)
	ECoverState GetCoverState() const {return CoverState;}

	UFUNCTION(BlueprintPure)
	bool IsUsable() const {return CoverState!=ECoverState::Destroyed;}
	
protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UPROPERTY(EditAnywhere, Category="PD|Cover|Slots")
	float CharacterClearance=70.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|Cover")
	TObjectPtr<UStaticMeshComponent> CoverMesh;
	
	UPROPERTY(EditAnywhere, Category="PD|Cover|Slots")
	TArray<FCoverSlot> Slots;
	
	UPROPERTY(EditDefaultsOnly, Category="PD|COVER|HP")
	float MaxHP=200.f;
	
	UPROPERTY(VisibleAnywhere, Category="PD|Cover|HP")
	float CurrentHP=200.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover|HP")
	float DamagedThreshold=0.5f;
private:
	ECoverState CoverState=ECoverState::Normal;
	void SetCoverState(ECoverState NewState);
	void OnDestroyed_Internal();
	//Test
	void NotifyOccupantsDestroyed();
	
	void GenerateSlots();
};
