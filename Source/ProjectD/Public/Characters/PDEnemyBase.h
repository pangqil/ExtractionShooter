#pragma once

#include "CoreMinimal.h"
#include "Characters/PDCharacterBase.h"
#include "PDEnemyBase.generated.h"

UENUM(BlueprintType)
enum class EPDEnemyState : uint8
{
	Idle,
	Chase,
	Attack,
	Dead
};

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase
{
	GENERATED_BODY()

public:
	APDEnemyBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI")
	EPDEnemyState CurrentState = EPDEnemyState::Idle;

public:
	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void SetEnemyState(EPDEnemyState NewState);

	FORCEINLINE EPDEnemyState GetEnemyState() const { return CurrentState; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnEnemyStateChanged(EPDEnemyState NewState);

	virtual void HandleDeath(AActor* Killer) override;
};
