#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Game/PDRaidStats.h"
#include "Type/Types.h"
#include "PDPlayerState.generated.h"

class APDPlayerCharacter;
class UPDEquipmentComponent;
class UPDInventoryComponent;
class UPDQuestComponent;
class UPDQuickSlotComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnRaidParticipationChanged, bool, bIsExtracted, bool, bIsRaidDead);

UCLASS(Blueprintable)
class PROJECTD_API APDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APDPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDQuickSlotComponent* GetQuickSlotComponent() const;

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDQuestComponent* GetQuestComponent() const { return QuestComponent; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	APDPlayerCharacter* GetPDPlayerCharacter() const;

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Save")
	void InitializePersistentData(const FPDPlayerData& InData);

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Save")
	FPDPlayerData GetPersistentData() const { return PersistentData; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Market")
	int32 GetTraderReputationExp() const;

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Market")
	int32 GetTraderReputationLevel() const;

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Market")
	void SetTraderReputation(int32 InLevel, int32 InExp);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Market")
	void AddTraderReputationExp(int32 Amount);

	const TArray<FPDInventorySlot>& GetRaidLoadout() const { return PersistentData.RaidLoadout; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	int32 GetRaidGold() const { return PersistentData.RaidGold; }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void ClearRaidLoadout();

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	bool IsExtracted() const { return bIsExtracted; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	bool IsRaidDead() const { return bIsRaidDead; }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void SetExtracted(bool bInExtracted);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void SetRaidDead(bool bInDead);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void ResetRaidParticipationState();

	// 결산/Hub UI가 동료의 추출·사망 전환을 실시간으로 받기 위한 알림. 서버/클라 모두 발화.
	UPROPERTY(BlueprintAssignable, Category="PD|PlayerState|Raid")
	FPDOnRaidParticipationChanged OnRaidParticipationChanged;

	// ─── Step 2-C: 라이드 결산 스탯 (서버 권한, 모든 클라에 리플리케이트) ─────
	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	const FPDRaidStats& GetRaidStats() const { return RaidStats; }

	void SetRaidStats(const FPDRaidStats& InStats);
	void AddKill();
	void SetSurvivalSeconds(float Seconds);
	void SetGoldDelta(int32 Delta);
	void SetItemDelta(int32 Delta);

	// 서버 전용: StartRaid 시점에 GameMode 가 호출하여 시작 스냅샷 저장. 결산에서 Delta 계산용.
	void CaptureInitialRaidSnapshot(int32 InGold, int32 InItemQuantity);
	int32 GetInitialRaidGold() const { return RaidInitialGold; }
	int32 GetInitialRaidItemQuantity() const { return RaidInitialItemQuantity; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Stash")
	const TArray<FPDInventorySlot>& GetPersistentStashItems() const { return PersistentData.StashItems; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Stash")
	int32 GetPersistentStashUpgradeLevel() const { return FMath::Max(0, PersistentData.StashUpgradeLevel); }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Stash")
	void SetPersistentStashSnapshot(const TArray<FPDInventorySlot>& InStashItems, int32 InUpgradeLevel);

	int32 CountPersistentStashItems(FName ItemID, bool bCountAnyItem) const;
	int32 RemovePersistentStashItems(FName ItemID, int32 Quantity, bool bRemoveAnyItem);
	void TransferInventoryToPersistentStash(const UPDInventoryComponent* InventoryComponent);

protected:
	virtual void BeginPlay() override;

private:
	void ApplyComponentDataDefaults();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|PlayerState|Data", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDQuickSlotComponent> QuickSlotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDQuestComponent> QuestComponent;

	UPROPERTY(ReplicatedUsing=OnRep_PersistentData, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Save", meta=(AllowPrivateAccess="true"))
	FPDPlayerData PersistentData;

	UPROPERTY(ReplicatedUsing=OnRep_RaidParticipation, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	bool bIsExtracted = false;

	UPROPERTY(ReplicatedUsing=OnRep_RaidParticipation, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	bool bIsRaidDead = false;

	// Step 2-C: 결산 위젯이 표시할 스탯. 전원이 서로 결과 보므로 COND_None 으로 모두에게 전파.
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	FPDRaidStats RaidStats;

	// 서버 전용 (UPROPERTY 아님 → 리플리케이트 안 됨). StartRaid 캡처 → 사망/추출 시 Delta 계산.
	int32 RaidInitialGold = 0;
	int32 RaidInitialItemQuantity = 0;

	UFUNCTION()
	void OnRep_PersistentData();

	UFUNCTION()
	void OnRep_RaidParticipation();
};
