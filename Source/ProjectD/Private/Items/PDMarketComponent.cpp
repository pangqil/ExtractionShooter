#include "Items/PDMarketComponent.h"

#include "Core/PDGameInstance.h"
#include "Engine/DataTable.h"

namespace
{
	const FPDItemData* FindPDMarketItemDataByID(const UDataTable* DataTable, const FName& ItemID)
	{
		if (!DataTable || ItemID.IsNone())
		{
			return nullptr;
		}

		TArray<FPDItemData*> Rows;
		DataTable->GetAllRows<FPDItemData>(TEXT("FindPDMarketItemDataByID"), Rows);

		for (const FPDItemData* Row : Rows)
		{
			if (Row && Row->ItemID == ItemID)
			{
				return Row;
			}
		}

		return nullptr;
	}

	const TCHAR* DefaultMarketLevelDataTablePath = TEXT("/Game/Main/Blueprints/Data/DT_MarketLevelData.DT_MarketLevelData");
}

UPDMarketComponent::UPDMarketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDMarketComponent::BeginPlay()
{
	Super::BeginPlay();
	SyncTraderReputationFromSave();
}

bool UPDMarketComponent::BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity)
{
	if (!BuyerInventory || !Goods.IsValidIndex(EntryIndex) || Quantity <= 0)
	{
		return false;
	}

	if (!CanBuyEntry(EntryIndex))
	{
		return false;
	}

	FPDMarketEntry& Entry = Goods[EntryIndex];
	FPDItemData ItemData;
	if (!ResolveEntryItemData(Entry, ItemData) || ItemData.ItemID.IsNone())
	{
		return false;
	}

	if (Entry.Stock >= 0 && Entry.Stock < Quantity)
	{
		return false;
	}

	const int32 UnitPrice = GetEntryUnitPrice(Entry);
	const int32 TotalPrice = UnitPrice * Quantity;

	if (!BuyerInventory->SpendGold(TotalPrice))
	{
		return false;
	}

	const int32 AddedQuantity = BuyerInventory->AddItemPartial(ItemData, Quantity);

	if (AddedQuantity != Quantity)
	{
		if (AddedQuantity > 0)
		{
			BuyerInventory->RemoveItem(ItemData.ItemID, AddedQuantity);
		}

		BuyerInventory->AddGold(TotalPrice);
		return false;
	}

	if (Entry.Stock >= 0)
	{
		Entry.Stock -= AddedQuantity;
	}

	AddTraderReputationExp(CalculateReputationReward(TotalPrice, BuyReputationExpPercent));

	OnMarketChanged.Broadcast();
	return true;
}

bool UPDMarketComponent::SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity)
{
	if (!SellerInventory || Quantity <= 0 || !SellerInventory->Items.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SellerInventory->Items[SlotIndex];
	if (SourceSlot.IsEmpty() || SourceSlot.ItemData.ItemID.IsNone())
	{
		return false;
	}

	const int32 SellQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	if (SellQuantity <= 0)
	{
		return false;
	}

	FPDMarketEntry* Entry = FindEntryByItemID(SourceSlot.ItemData.ItemID);
	if (SourceSlot.ItemData.bIsQuestItem)
	{
		return false;
	}

	const int32 UnitPrice = GetItemSellPrice(SourceSlot.ItemData);
	const int32 TotalPrice = UnitPrice * SellQuantity;

	FPDInventorySlot& MutableSourceSlot = SellerInventory->Items[SlotIndex];
	MutableSourceSlot.Quantity -= SellQuantity;
	if (MutableSourceSlot.Quantity <= 0)
	{
		MutableSourceSlot.Clear();
	}
	SellerInventory->OnInventoryChanged.Broadcast();

	SellerInventory->AddGold(TotalPrice);

	if (Entry && Entry->Stock >= 0)
	{
		Entry->Stock += SellQuantity;
	}

	AddTraderReputationExp(CalculateReputationReward(TotalPrice, SellReputationExpPercent));

	OnMarketChanged.Broadcast();
	return true;
}

bool UPDMarketComponent::ResolveEntryItemData(const FPDMarketEntry& Entry, FPDItemData& OutItemData) const
{
	OutItemData = FPDItemData();

	if (!Entry.ItemDataTable || Entry.ItemRowName.IsNone())
	{
		return false;
	}

	const FPDItemData* Row = FindPDMarketItemDataByID(Entry.ItemDataTable, Entry.ItemRowName);
	if (!Row)
	{
		return false;
	}

	OutItemData = *Row;
	if (OutItemData.ItemID.IsNone())
	{
		OutItemData.ItemID = Entry.ItemRowName;
	}

	return true;
}

int32 UPDMarketComponent::GetEntryUnitPrice(const FPDMarketEntry& Entry) const
{
	if (Entry.OverridePrice >= 0)
	{
		return Entry.OverridePrice;
	}

	FPDItemData ItemData;
	return ResolveEntryItemData(Entry, ItemData) ? GetItemBuyPrice(ItemData) : 0;
}

int32 UPDMarketComponent::GetItemBuyPrice(const FPDItemData& ItemData) const
{
	return FMath::Max(0, ItemData.Price);
}

int32 UPDMarketComponent::GetItemSellPrice(const FPDItemData& ItemData) const
{
	const int32 BuyPrice = GetItemBuyPrice(ItemData);
	if (BuyPrice <= 0 || SellPriceRate <= 0.f)
	{
		return 0;
	}

	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(BuyPrice) * SellPriceRate));
}

bool UPDMarketComponent::CanBuyEntry(int32 EntryIndex) const
{
	if (!Goods.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FPDItemData ItemData;
	return ResolveEntryItemData(Goods[EntryIndex], ItemData) && CanBuyItemData(ItemData);
}

bool UPDMarketComponent::CanBuyItemData(const FPDItemData& ItemData) const
{
	return static_cast<uint8>(ItemData.ItemGrade) <= static_cast<uint8>(GetMaxPurchasableGradeForLevel(TraderReputationLevel));
}

bool UPDMarketComponent::ShouldShowEntry(int32 EntryIndex) const
{
	const int32 RequiredLevel = GetRequiredTraderLevelForEntry(EntryIndex);
	return RequiredLevel > 0 && RequiredLevel <= TraderReputationLevel + 1;
}

int32 UPDMarketComponent::GetRequiredTraderLevelForGrade(EPDItemGrade ItemGrade) const
{
	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable)
	{
		return 1;
	}

	TArray<FPDMarketLevelData*> LevelRows;
	LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("GetRequiredTraderLevelForGrade"), LevelRows);

	int32 BestLevel = INDEX_NONE;
	for (const FPDMarketLevelData* LevelData : LevelRows)
	{
		if (!LevelData)
		{
			continue;
		}

		if (static_cast<uint8>(LevelData->MaxPurchasableGrade) >= static_cast<uint8>(ItemGrade))
		{
			if (BestLevel == INDEX_NONE || LevelData->Level < BestLevel)
			{
				BestLevel = LevelData->Level;
			}
		}
	}

	return BestLevel == INDEX_NONE ? 1 : BestLevel;
}

int32 UPDMarketComponent::GetRequiredTraderLevelForEntry(int32 EntryIndex) const
{
	if (!Goods.IsValidIndex(EntryIndex))
	{
		return INDEX_NONE;
	}

	FPDItemData ItemData;
	if (!ResolveEntryItemData(Goods[EntryIndex], ItemData))
	{
		return INDEX_NONE;
	}

	return GetRequiredTraderLevelForGrade(ItemData.ItemGrade);
}

EPDItemGrade UPDMarketComponent::GetMaxPurchasableGradeForLevel(int32 Level) const
{
	const FPDMarketLevelData* ExactLevelData = FindMarketLevelDataByLevel(Level);
	if (ExactLevelData)
	{
		return ExactLevelData->MaxPurchasableGrade;
	}

	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable)
	{
		return EPDItemGrade::Grade1;
	}

	// Fallback: 지정 레벨 Row가 없으면 현재 레벨 이하 중 가장 높은 Row를 사용합니다.
	TArray<FPDMarketLevelData*> LevelRows;
	LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("GetMaxPurchasableGradeForLevel"), LevelRows);

	EPDItemGrade Result = EPDItemGrade::Grade1;
	int32 BestLevel = 0;
	for (const FPDMarketLevelData* LevelData : LevelRows)
	{
		if (!LevelData)
		{
			continue;
		}

		if (LevelData->Level <= Level && LevelData->Level >= BestLevel)
		{
			BestLevel = LevelData->Level;
			Result = LevelData->MaxPurchasableGrade;
		}
	}

	return Result;
}


int32 UPDMarketComponent::GetCurrentTraderLevelRequiredExp() const
{
	const FPDMarketLevelData* CurrentLevelData = FindMarketLevelDataByLevel(FMath::Max(1, TraderReputationLevel));
	return CurrentLevelData ? FMath::Max(0, CurrentLevelData->RequiredExp) : 0;
}

int32 UPDMarketComponent::GetNextTraderLevelRequiredExp() const
{
	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDMarketComponent: MarketLevelDataTable is not set and default DT_MarketLevelData could not be loaded."));
		return INDEX_NONE;
	}

	TArray<FPDMarketLevelData*> LevelRows;
	LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("GetNextTraderLevelRequiredExp"), LevelRows);

	int32 NextRequiredExp = INDEX_NONE;
	for (const FPDMarketLevelData* LevelData : LevelRows)
	{
		if (!LevelData)
		{
			continue;
		}

		// RequiredExp는 누적 경험치입니다. 현재 EXP보다 큰 가장 가까운 값을 다음 목표로 사용합니다.
		if (LevelData->RequiredExp > TraderReputationExp &&
			(NextRequiredExp == INDEX_NONE || LevelData->RequiredExp < NextRequiredExp))
		{
			NextRequiredExp = LevelData->RequiredExp;
		}
	}

	return NextRequiredExp;
}

int32 UPDMarketComponent::GetCurrentTraderLevelDisplayExp() const
{
	const int32 CurrentLevelStartExp = GetCurrentTraderLevelRequiredExp();
	return FMath::Max(0, TraderReputationExp - CurrentLevelStartExp);
}

int32 UPDMarketComponent::GetNextTraderLevelDisplayRequiredExp() const
{
	const int32 CurrentLevelStartExp = GetCurrentTraderLevelRequiredExp();
	const int32 NextRequiredExp = GetNextTraderLevelRequiredExp();

	if (NextRequiredExp == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return FMath::Max(0, NextRequiredExp - CurrentLevelStartExp);
}

void UPDMarketComponent::AddTraderReputationExp(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	TraderReputationExp += Amount;
	RecalculateTraderReputationLevel();
	SaveTraderReputationToSave();
	OnTraderReputationChanged.Broadcast(TraderReputationLevel, TraderReputationExp);
	OnMarketChanged.Broadcast();
}

void UPDMarketComponent::SyncTraderReputationFromSave()
{
	LoadTraderReputationFromSave();
	RecalculateTraderReputationLevel();
	SaveTraderReputationToSave();
	OnTraderReputationChanged.Broadcast(TraderReputationLevel, TraderReputationExp);
}

void UPDMarketComponent::RecalculateTraderReputationLevel()
{
	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable)
	{
		TraderReputationLevel = FMath::Max(1, TraderReputationLevel);
		return;
	}

	TArray<FPDMarketLevelData*> LevelRows;
	LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("RecalculateTraderReputationLevel"), LevelRows);

	int32 NewLevel = 1;
	int32 BestRequiredExp = INDEX_NONE;
	for (const FPDMarketLevelData* LevelData : LevelRows)
	{
		if (!LevelData)
		{
			continue;
		}

		if (TraderReputationExp >= LevelData->RequiredExp &&
			(BestRequiredExp == INDEX_NONE || LevelData->RequiredExp >= BestRequiredExp))
		{
			BestRequiredExp = LevelData->RequiredExp;
			NewLevel = FMath::Max(1, LevelData->Level);
		}
	}

	TraderReputationLevel = NewLevel;
}

const UDataTable* UPDMarketComponent::GetResolvedMarketLevelDataTable() const
{
	if (MarketLevelDataTable)
	{
		return MarketLevelDataTable;
	}

	// 마켓 Actor 컴포넌트에 DT 지정이 누락돼도 공통 DT를 사용하도록 fallback 처리합니다.
	return LoadObject<UDataTable>(nullptr, DefaultMarketLevelDataTablePath);
}

const FPDMarketLevelData* UPDMarketComponent::FindMarketLevelDataByLevel(int32 Level) const
{
	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable || Level <= 0)
	{
		return nullptr;
	}

	// 권장 RowName: 1, 2, 3 ...
	const FName NumericRowName(*FString::FromInt(Level));
	if (const FPDMarketLevelData* Row = LevelTable->FindRow<FPDMarketLevelData>(NumericRowName, TEXT("FindMarketLevelDataByLevel"), false))
	{
		return Row;
	}

	// 호환용 fallback: 기존 Level_1, Level_2 ... RowName도 지원합니다.
	const FName LegacyRowName(*FString::Printf(TEXT("Level_%d"), Level));
	if (const FPDMarketLevelData* Row = LevelTable->FindRow<FPDMarketLevelData>(LegacyRowName, TEXT("FindMarketLevelDataByLevel"), false))
	{
		return Row;
	}

	// 최종 fallback: RowName이 달라도 Level 컬럼 값이 맞으면 사용합니다.
	TArray<FPDMarketLevelData*> LevelRows;
	LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("FindMarketLevelDataByLevel"), LevelRows);
	for (const FPDMarketLevelData* LevelData : LevelRows)
	{
		if (LevelData && LevelData->Level == Level)
		{
			return LevelData;
		}
	}

	return nullptr;
}

FPDMarketEntry* UPDMarketComponent::FindEntryByItemID(FName ItemID)
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}

	for (FPDMarketEntry& Entry : Goods)
	{
		FPDItemData EntryItemData;
		if (ResolveEntryItemData(Entry, EntryItemData) && EntryItemData.ItemID == ItemID)
		{
			return &Entry;
		}
	}

	return nullptr;
}

void UPDMarketComponent::LoadTraderReputationFromSave()
{
	if (const UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr)
	{
		TraderReputationExp = GI->GetTraderReputationExp();
		TraderReputationLevel = GI->GetTraderReputationLevel();
	}
}

void UPDMarketComponent::SaveTraderReputationToSave() const
{
	if (UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr)
	{
		GI->SetTraderReputation(TraderReputationLevel, TraderReputationExp);
	}
}

int32 UPDMarketComponent::CalculateReputationReward(int32 TotalPrice, int32 Percent) const
{
	if (TotalPrice <= 0 || Percent <= 0)
	{
		return 0;
	}

	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(TotalPrice * Percent) / 100.f));
}
