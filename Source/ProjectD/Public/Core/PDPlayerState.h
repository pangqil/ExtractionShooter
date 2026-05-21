#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Type/Types.h"
#include "PDPlayerState.generated.h"

class APDPlayerCharacter;
class UPDEquipmentComponent;
class UPDInventoryComponent;
class UPDQuestComponent;
class UPDQuickSlotComponent;

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
	UPDQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

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

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Stash")
	const TArray<FPDInventorySlot>& GetPersistentStashItems() const { return PersistentData.StashItems; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Stash")
	int32 GetPersistentStashUpgradeLevel() const { return FMath::Max(0, PersistentData.StashUpgradeLevel); }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Stash")
	void SetPersistentStashSnapshot(const TArray<FPDInventorySlot>& InStashItems, int32 InUpgradeLevel);

	int32 CountPersistentStashItems(FName ItemID, bool bCountAnyItem) const;
	int32 RemovePersistentStashItems(FName ItemID, int32 Quantity, bool bRemoveAnyItem);
	void TransferInventoryToPersistentStash(const UPDInventoryComponent* InventoryComponent);

private:
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

	UFUNCTION()
	void OnRep_PersistentData();
};
