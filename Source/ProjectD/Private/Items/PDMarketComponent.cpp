#include "Items/PDMarketComponent.h"

#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

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
	SetIsReplicatedByDefault(true);
}

void UPDMarketComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDMarketComponent, Goods);
	DOREPLIFETIME(UPDMarketComponent, TraderReputationExp);
	DOREPLIFETIME(UPDMarketComponent, TraderReputationLevel);
}

void UPDMarketComponent::OnRep_MarketData()
{
	OnTraderReputationChanged.Broadcast(TraderReputationLevel, TraderReputationExp);
	OnMarketChanged.Broadcast();
}

void UPDMarketComponent::BeginPlay()
{
	Super::BeginPlay();
	SyncTraderReputationFromSave();
}

bool UPDMarketComponent::BuyEntry(UPDInventoryComponent* BuyerInventory, int32 EntryIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (const UWorld* World = GetWorld())
		{
			if (APDPlayerController* PC = Cast<APDPlayerController>(World->GetFirstPlayerController()))
			{
				PC->ServerBuyMarketEntry(this, EntryIndex, Quantity);
				return true;
			}
		}
		return false;
	}

	if (!BuyerInventory || !Goods.IsValidIndex(EntryIndex) || Quantity <= 0)
	{
		return false;
	}

	if (!CanBuyEntryForInventory(EntryIndex, BuyerInventory))
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

	AddTraderReputationExpForInventory(BuyerInventory, CalculateReputationReward(TotalPrice, BuyReputationExpPercent));

	OnMarketChanged.Broadcast();
	return true;
}

bool UPDMarketComponent::SellInventorySlot(UPDInventoryComponent* SellerInventory, int32 SlotIndex, int32 Quantity)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		if (const UWorld* World = GetWorld())
		{
			if (APDPlayerController* PC = Cast<APDPlayerController>(World->GetFirstPlayerController()))
			{
				PC->ServerSellInventorySlotToMarket(this, SlotIndex, Quantity);
				return true;
			}
		}
		return false;
	}

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

	AddTraderReputationExpForInventory(SellerInventory, CalculateReputationReward(TotalPrice, SellReputationExpPercent));

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
	return CanBuyEntryForInventory(EntryIndex, nullptr);
}

bool UPDMarketComponent::CanBuyEntryForInventory(int32 EntryIndex, const UPDInventoryComponent* BuyerInventory) const
{
	if (!Goods.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FPDItemData ItemData;
	return ResolveEntryItemData(Goods[EntryIndex], ItemData) && CanBuyItemDataForInventory(ItemData, BuyerInventory);
}

bool UPDMarketComponent::CanBuyItemData(const FPDItemData& ItemData) const
{
	return CanBuyItemDataForInventory(ItemData, nullptr);
}

bool UPDMarketComponent::CanBuyItemDataForInventory(const FPDItemData& ItemData, const UPDInventoryComponent* BuyerInventory) const
{
	return static_cast<uint8>(ItemData.ItemGrade) <= static_cast<uint8>(GetMaxPurchasableGradeForLevel(GetTraderReputationLevelForInventory(BuyerInventory)));
}

bool UPDMarketComponent::ShouldShowEntry(int32 EntryIndex) const
{
	return ShouldShowEntryForInventory(EntryIndex, nullptr);
}

bool UPDMarketComponent::ShouldShowEntryForInventory(int32 EntryIndex, const UPDInventoryComponent* BuyerInventory) const
{
	const int32 RequiredLevel = GetRequiredTraderLevelForEntry(EntryIndex);
	return RequiredLevel > 0 && RequiredLevel <= GetTraderReputationLevelForInventory(BuyerInventory) + 1;
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
	return GetCurrentTraderLevelDisplayExpForInventory(nullptr);
}

int32 UPDMarketComponent::GetCurrentTraderLevelDisplayExpForInventory(const UPDInventoryComponent* BuyerInventory) const
{
	const FPDMarketLevelData* CurrentLevelData = FindMarketLevelDataByLevel(GetTraderReputationLevelForInventory(BuyerInventory));
	const int32 CurrentLevelStartExp = CurrentLevelData ? FMath::Max(0, CurrentLevelData->RequiredExp) : 0;
	return FMath::Max(0, GetTraderReputationExpForInventory(BuyerInventory) - CurrentLevelStartExp);
}

int32 UPDMarketComponent::GetNextTraderLevelDisplayRequiredExp() const
{
	return GetNextTraderLevelDisplayRequiredExpForInventory(nullptr);
}

int32 UPDMarketComponent::GetNextTraderLevelDisplayRequiredExpForInventory(const UPDInventoryComponent* BuyerInventory) const
{
	const int32 ReputationExp = GetTraderReputationExpForInventory(BuyerInventory);
	const FPDMarketLevelData* CurrentLevelData = FindMarketLevelDataByLevel(GetTraderReputationLevelForInventory(BuyerInventory));
	const int32 CurrentLevelStartExp = CurrentLevelData ? FMath::Max(0, CurrentLevelData->RequiredExp) : 0;
	int32 NextRequiredExp = INDEX_NONE;

	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (LevelTable)
	{
		TArray<FPDMarketLevelData*> LevelRows;
		LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("GetNextTraderLevelDisplayRequiredExpForInventory"), LevelRows);

		for (const FPDMarketLevelData* LevelData : LevelRows)
		{
			if (LevelData && LevelData->RequiredExp > ReputationExp &&
				(NextRequiredExp == INDEX_NONE || LevelData->RequiredExp < NextRequiredExp))
			{
				NextRequiredExp = LevelData->RequiredExp;
			}
		}
	}

	if (NextRequiredExp == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return FMath::Max(0, NextRequiredExp - CurrentLevelStartExp);
}

void UPDMarketComponent::AddTraderReputationExp(int32 Amount)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	LoadTraderReputationFromSave();
	RecalculateTraderReputationLevel();
	SaveTraderReputationToSave();
	OnTraderReputationChanged.Broadcast(TraderReputationLevel, TraderReputationExp);
}

void UPDMarketComponent::RecalculateTraderReputationLevel()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

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


	return LoadObject<UDataTable>(nullptr, DefaultMarketLevelDataTablePath);
}

const FPDMarketLevelData* UPDMarketComponent::FindMarketLevelDataByLevel(int32 Level) const
{
	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable || Level <= 0)
	{
		return nullptr;
	}


	const FName NumericRowName(*FString::FromInt(Level));
	if (const FPDMarketLevelData* Row = LevelTable->FindRow<FPDMarketLevelData>(NumericRowName, TEXT("FindMarketLevelDataByLevel"), false))
	{
		return Row;
	}


	const FName LegacyRowName(*FString::Printf(TEXT("Level_%d"), Level));
	if (const FPDMarketLevelData* Row = LevelTable->FindRow<FPDMarketLevelData>(LegacyRowName, TEXT("FindMarketLevelDataByLevel"), false))
	{
		return Row;
	}


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

APDPlayerState* UPDMarketComponent::GetPlayerStateFromInventory(const UPDInventoryComponent* InventoryComponent) const
{
	return InventoryComponent ? Cast<APDPlayerState>(InventoryComponent->GetOwner()) : nullptr;
}

int32 UPDMarketComponent::GetTraderReputationExpForInventory(const UPDInventoryComponent* InventoryComponent) const
{
	if (const APDPlayerState* PDPlayerState = GetPlayerStateFromInventory(InventoryComponent))
	{
		return PDPlayerState->GetTraderReputationExp();
	}

	return FMath::Max(0, TraderReputationExp);
}

int32 UPDMarketComponent::GetTraderReputationLevelForInventory(const UPDInventoryComponent* InventoryComponent) const
{
	if (const APDPlayerState* PDPlayerState = GetPlayerStateFromInventory(InventoryComponent))
	{
		return CalculateTraderReputationLevelFromExp(PDPlayerState->GetTraderReputationExp());
	}

	return FMath::Max(1, TraderReputationLevel);
}

int32 UPDMarketComponent::CalculateTraderReputationLevelFromExp(int32 ReputationExp) const
{
	const UDataTable* LevelTable = GetResolvedMarketLevelDataTable();
	if (!LevelTable)
	{
		return 1;
	}

	TArray<FPDMarketLevelData*> LevelRows;
	LevelTable->GetAllRows<FPDMarketLevelData>(TEXT("CalculateTraderReputationLevelFromExp"), LevelRows);

	int32 NewLevel = 1;
	int32 BestRequiredExp = INDEX_NONE;
	for (const FPDMarketLevelData* LevelData : LevelRows)
	{
		if (!LevelData)
		{
			continue;
		}

		if (ReputationExp >= LevelData->RequiredExp &&
			(BestRequiredExp == INDEX_NONE || LevelData->RequiredExp >= BestRequiredExp))
		{
			BestRequiredExp = LevelData->RequiredExp;
			NewLevel = FMath::Max(1, LevelData->Level);
		}
	}

	return NewLevel;
}

void UPDMarketComponent::AddTraderReputationExpForInventory(UPDInventoryComponent* InventoryComponent, int32 Amount)
{
	if (APDPlayerState* PDPlayerState = GetPlayerStateFromInventory(InventoryComponent))
	{
		PDPlayerState->AddTraderReputationExp(Amount);
		PDPlayerState->SetTraderReputation(
			CalculateTraderReputationLevelFromExp(PDPlayerState->GetTraderReputationExp()),
			PDPlayerState->GetTraderReputationExp());

		TraderReputationExp = PDPlayerState->GetTraderReputationExp();
		TraderReputationLevel = PDPlayerState->GetTraderReputationLevel();
		OnTraderReputationChanged.Broadcast(TraderReputationLevel, TraderReputationExp);
		OnMarketChanged.Broadcast();
		return;
	}

	AddTraderReputationExp(Amount);
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
