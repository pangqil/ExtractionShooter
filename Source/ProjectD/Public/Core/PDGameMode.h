#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Type/Types.h"
#include "PDGameMode.generated.h"

class APDPlayerController;
struct FPDPlayerRaidEntryData;

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

	// 결산 위젯 페이드 종료 시 클라이언트가 PC->Server_RequestBaseTravel 경유로 호출.
	// 모든 PC가 ACK하거나 TravelGateTimeoutSeconds 경과 시 ServerTravel 발동.
	void NotifyPlayerReadyForTravel(APlayerController* PC);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;

	// 결산 위젯이 떠 있는 동안 전체 PC가 좌클릭(ACK)하지 않더라도 강제 트래블되는 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid", meta=(ClampMin="1.0", ForceUnits="s"))
	float TravelGateTimeoutSeconds = 20.f;

	void SetRaidState(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidEnded(bool bSuccess);

	void RequestBaseTravel();

private:
	// Step 2-B: 사망 시 죽은 PC를 첫 생존자로 관전 전환, 기존 관전자들 중 죽은 PC를
	// 보고 있던 사람들은 다음 생존자로 순환. AffectedPC 는 방금 죽거나 추출된 PC.
	void HandleSpectatorTransitionForDeath(APlayerController* DeadPC);
	void NotifyOthersWatchingFinalized(APlayerController* AffectedPC);
	APlayerController* FindFirstAliveSpectateTarget(APlayerController* ExcludePC) const;

	// Step 2-C: 결산 스탯 추적.
	void CaptureInitialRaidSnapshotFor(APlayerController* PC);
	void FinalizeRaidStatsOnDeath(APlayerController* DeadPC);
	void FinalizeRaidStatsOnExtraction(APlayerController* ExtractedPC);
	void CreditKillToShooter(AActor* Killer, APlayerController* Victim);
	float GetCurrentRaidElapsedSeconds() const;
	int32 CountInventoryItemQuantity(class UPDInventoryComponent* Inventory) const;

	// 옵션 B: 결산 위젯 entries 서버 빌드 (GameState->PlayerArray + PDPlayerState).
	void BuildRaidEndEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const;

	// 서버 권한 GameMode 만 사용. RaidStats.SurvivalSeconds 계산 기준점.
	float RaidStartServerTime = 0.f;

	void InitializePlayerStateFromSave(APlayerController* PC);
	void SavePlayerStateToDisk(APlayerController* PC);
	void InitializePlayerInventoryFromLoadout(APlayerController* PC);
	void TransferPlayerInventoryToStash(APlayerController* PC);

	// 모든 참가자가 Dead 또는 Extracted 상태가 되었는지 검사하고, 그 경우 EndRaid 발동.
	void EvaluateRaidEnd();
	// 한 PC 의 추출 후처리(인벤토리 이체, 상태 저장, SecureContainer 보존). 멀티 자체는 그대로 진행.
	void ProcessExtractionForPlayer(APlayerController* PC);
	// 한 PC 의 사망 후처리(인벤토리 손실 = 이체 안 함, 상태만 저장).
	void ProcessDeathForPlayer(APlayerController* PC);

	bool AreAllPlayersReadyForTravel() const;
	void OnTravelTimeoutExpired();

	int32 CountAlivePlayers() const;
	int32 CountExtractedPlayers() const;
	int32 CountDeadPlayers() const;

	TArray<TWeakObjectPtr<APlayerController>> ReadyPlayersForTravel;
	FTimerHandle TravelTimeoutTimerHandle;
	bool bTravelStarted = false;
};
