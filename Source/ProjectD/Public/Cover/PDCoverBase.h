#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDCoverBase.generated.h"

UENUM(BlueprintType)
enum class ECoverState : uint8
{
	Normal,
	Damaged,
	Destroyed
};

UCLASS()
class PROJECTD_API APDCoverBase : public AActor
{
	GENERATED_BODY()

public:
	APDCoverBase();
	
	FVector GetSnapLocation(AActor* Requester) const;
	FRotator GetSnapRotation(AActor* Requester) const;

	bool TryOccupy(AActor* Requester);
	void Release(AActor* Requester);

	void TakeCoverDamage(float Damage);

	UFUNCTION(BlueprintPure)
	ECoverState GetCoverState() const { return CoverState; }

	UFUNCTION(BlueprintPure)
	bool IsUsable() const { return CoverState != ECoverState::Destroyed; }

	UFUNCTION(BlueprintPure)
	bool IsOccupied() const { return Occupant.IsValid(); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|Cover")
	TObjectPtr<UStaticMeshComponent> CoverMesh;
	
	UPROPERTY(EditDefaultsOnly, Category="PD|Cover")
	float CharacterClearance = 70.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover|HP")
	float MaxHP = 200.f;

	UPROPERTY(VisibleAnywhere, Category="PD|Cover|HP")
	float CurrentHP = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|Cover|HP")
	float DamagedThreshold = 0.5f;

private:
	ECoverState CoverState = ECoverState::Normal;
	TWeakObjectPtr<AActor> Occupant;

	void SetCoverState(ECoverState NewState);
	void OnDestroyed_Internal();
};
