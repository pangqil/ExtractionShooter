#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Type/Types.h"
#include "PDGameInstance.generated.h"

UCLASS()
class PROJECTD_API UPDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SetPlayerData(const FPDPlayerData& InData);
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	FPDPlayerData GetPlayerData() const { return PlayerData; }

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashItems(const TArray<FPDInventorySlot>& InStashItems);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	const TArray<FPDInventorySlot>& GetStashItems() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashUpgradeLevel(int32 InUpgradeLevel);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	int32 GetStashUpgradeLevel() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetTraderReputation(int32 InLevel, int32 InExp);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetTraderReputationExp(int32 InExp);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetTraderReputationLevel(int32 InLevel);

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	int32 GetTraderReputationExp() const;

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	int32 GetTraderReputationLevel() const;
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SaveToDisk();
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void LoadFromDisk();

protected:
	UPROPERTY()
	FPDPlayerData PlayerData;
};
