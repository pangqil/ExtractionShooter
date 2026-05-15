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
}

UPDMarketComponent::UPDMarketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDMarketComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultReputationLevels();
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
	int32 BestLevel = INDEX_NONE;
	for (const FPDTraderReputationLevelData& LevelData : ReputationLevels)
	{
		if (static_cast<uint8>(LevelData.MaxPurchasableGrade) >= static_cast<uint8>(ItemGrade))
		{
			if (BestLevel == INDEX_NONE || LevelData.Level < BestLevel)
			{
				BestLevel = LevelData.Level;
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
	EPDItemGrade Result = EPDItemGrade::Grade1;
	int32 BestLevel = 0;

	for (const FPDTraderReputationLevelData& LevelData : ReputationLevels)
	{
		if (LevelData.Level <= Level && LevelData.Level >= BestLevel)
		{
			BestLevel = LevelData.Level;
			Result = LevelData.MaxPurchasableGrade;
		}
	}

	return Result;
}

int32 UPDMarketComponent::GetNextTraderLevelRequiredExp() const
{
	int32 NextRequiredExp = INDEX_NONE;
	for (const FPDTraderReputationLevelData& LevelData : ReputationLevels)
	{
		if (LevelData.Level == TraderReputationLevel + 1)
		{
			NextRequiredExp = LevelData.RequiredExp;
			break;
		}
	}

	return NextRequiredExp;
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
	InitializeDefaultReputationLevels();

	int32 NewLevel = 1;
	int32 BestRequiredExp = 0;
	for (const FPDTraderReputationLevelData& LevelData : ReputationLevels)
	{
		if (TraderReputationExp >= LevelData.RequiredExp && LevelData.RequiredExp >= BestRequiredExp)
		{
			BestRequiredExp = LevelData.RequiredExp;
			NewLevel = FMath::Max(1, LevelData.Level);
		}
	}

	TraderReputationLevel = NewLevel;
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

void UPDMarketComponent::InitializeDefaultReputationLevels()
{
	if (ReputationLevels.Num() > 0)
	{
		return;
	}

	ReputationLevels.Reset();

	FPDTraderReputationLevelData LevelData;
	LevelData.Level = 1;
	LevelData.RequiredExp = 0;
	LevelData.MaxPurchasableGrade = EPDItemGrade::Grade1;
	ReputationLevels.Add(LevelData);

	LevelData.Level = 2;
	LevelData.RequiredExp = 1000;
	LevelData.MaxPurchasableGrade = EPDItemGrade::Grade2;
	ReputationLevels.Add(LevelData);

	LevelData.Level = 3;
	LevelData.RequiredExp = 3500;
	LevelData.MaxPurchasableGrade = EPDItemGrade::Grade3;
	ReputationLevels.Add(LevelData);

	LevelData.Level = 4;
	LevelData.RequiredExp = 8000;
	LevelData.MaxPurchasableGrade = EPDItemGrade::Grade4;
	ReputationLevels.Add(LevelData);

	LevelData.Level = 5;
	LevelData.RequiredExp = 15000;
	LevelData.MaxPurchasableGrade = EPDItemGrade::Grade5;
	ReputationLevels.Add(LevelData);
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
