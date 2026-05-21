#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Type/Types.h"
#include "PDGameMode.generated.h"

class APDPlayerController;

UCLASS(abstract)
class PROJECTD_API APDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void StartRaid();

	UFUNCTION(BlueprintCallable, Category="PD|Raid|Travel")
	void TravelToRaidLevel(FName RaidMapName);

	UFUNCTION(BlueprintCallable, Category="PD|Raid|Travel")
	void TravelToBaseLevel(bool bMarkResetPending);

	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void RequestExtraction(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void EndRaid(bool bSuccess);

	void OnPlayerDied(APlayerController* PC, AActor* Killer);
	FORCEINLINE ERaidState GetRaidState() { return CurrentRaidState; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid", meta=(ClampMin="0.0"))
	float DeathToTravelDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid", meta=(ClampMin="0.0"))
	float ExtractionToTravelDelay = 1.0f;

	void SetRaidState(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidEnded(bool bSuccess);

private:
	void HandleReturnToBaseTravel();
	void ScheduleReturnToBaseTravel(float Delay);
	FTimerHandle ReturnToBaseTravelTimerHandle;

	void InitializePlayerStateFromSave(APlayerController* PC);
	void SavePlayerStateToDisk(APlayerController* PC);
	void InitializePlayerInventoryFromLoadout(APlayerController* PC);
	void TransferPlayerInventoryToStash(APlayerController* PC);
};
