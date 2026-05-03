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
	
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void StartRaid();
	
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void RequestExtraction(APlayerController* PC);
	
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void EndRaid(bool bSuccess);
	
	void OnPlayerDied(APlayerController* PC, AActor* Killer);
	FORCEINLINE ERaidState GetRaidState() { return CurrentRaidState; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;
	
	void SetRaidState(ERaidState NewState);
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);
	
	
};
