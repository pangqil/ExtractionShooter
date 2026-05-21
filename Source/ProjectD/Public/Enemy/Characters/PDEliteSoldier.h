#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "PDEliteSoldier.generated.h"

class APDCoverBase;

UCLASS(Blueprintable)
class PROJECTD_API APDEliteSoldier : public APDSoldier
{
	GENERATED_BODY()

public:
	APDEliteSoldier();

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE APDCoverBase* GetCurrentCover() const { return CurrentCover.Get(); }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsInCover() const { return bIsInCover; }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsPeeking() const { return bIsPeeking; }

	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetInCover(APDCoverBase* NewCover);

	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetPeeking(bool bPeek);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Elite|Grenade")
	void ThrowGrenadeAt(const FVector& TargetLocation);

protected:
	virtual void OnEnterState_Dead() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Grenade")
	TSubclassOf<AActor> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Grenade")
	FName GrenadeSpawnSocketName = TEXT("GrenadeSocket");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsInCover = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsPeeking = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<APDCoverBase> CurrentCover;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEnterCover(APDCoverBase* Cover);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnExitCover();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnStartPeek();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEndPeek();
};
