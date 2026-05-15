#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Items/PDInventoryComponent.h"
#include "PDMarketComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnMarketChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnTraderReputationChanged, int32, NewLevel, int32, NewExp);

USTRUCT(BlueprintType)
struct FPDMarketEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	TObjectPtr<UDataTable> ItemDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	FName ItemRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 Stock = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 OverridePrice = -1;
};

USTRUCT(BlueprintType)
struct FPDTraderReputationLevelData
{
	GENERATED_BODY()

	// 1부터 시작하는 상인 평판 레벨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 Level = 1;

	// 해당 레벨에 도달하기 위해 필요한 누적 평판 경험치.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	int32 RequiredExp = 0;

	// 해당 레벨에서 구매 가능한 최대 아이템 등급.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	EPDItemGrade MaxPurchasableGrade = EPDItemGrade::Grade1;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDMarketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDMarketComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	TArray<FPDMarketEntry> Goods;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Reputation")
	TArray<FPDTraderReputationLevelData> ReputationLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Reputation", meta=(ClampMin="0"))
	int32 TraderReputationExp = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Reputation", meta=(ClampMin="1"))
	int32 TraderReputationLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Reputation", meta=(ClampMin="0"))
	int32 BuyReputationExpPercent = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Reputation", meta=(ClampMin="0"))
	int32 SellReputationExpPercent = 2;

	// 아이템 Price 기준 판매 비율. 기본값 0.35 = 구매 기준가의 35%.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market|Price", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SellPriceRate = 0.35f;

	UPROPERTY(BlueprintAssignable, Category="PD|Market")
	FPDOnMarketChanged OnMarketChanged;

	UPROPERTY(BlueprintAssignable, Category="PD|Market|Reputation")
	FPDOnTraderReputationChanged OnTraderReputationChanged;

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category="PD|Market")
	bool ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetEntryUnitPrice(const FPDMarketEntry& Entry) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetItemBuyPrice(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market")
	int32 GetItemSellPrice(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	bool CanBuyEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	bool CanBuyItemData(const FPDItemData& ItemData) const;

	// 현재 레벨에서 구매 가능하거나 다음 레벨에 잠금 표시(????)할 상품이면 true.
	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	bool ShouldShowEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	int32 GetRequiredTraderLevelForGrade(EPDItemGrade ItemGrade) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	int32 GetRequiredTraderLevelForEntry(int32 EntryIndex) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	EPDItemGrade GetMaxPurchasableGradeForLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category="PD|Market|Reputation")
	int32 GetNextTraderLevelRequiredExp() const;

	UFUNCTION(BlueprintCallable, Category="PD|Market|Reputation")
	void AddTraderReputationExp(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="PD|Market|Reputation")
	void SyncTraderReputationFromSave();

	UFUNCTION(BlueprintCallable, Category="PD|Market|Reputation")
	void RecalculateTraderReputationLevel();

private:
	FPDMarketEntry* FindEntryByItemID(FName ItemID);
	void InitializeDefaultReputationLevels();
	void LoadTraderReputationFromSave();
	void SaveTraderReputationToSave() const;
	int32 CalculateReputationReward(int32 TotalPrice, int32 Percent) const;
};
