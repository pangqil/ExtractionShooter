#include "Items/PDMarketComponent.h"

UPDMarketComponent::UPDMarketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDMarketComponent::BuyItem(UPDInventoryComponent* BuyerInventory, FName ItemID, int32 Quantity)
{
	if (!BuyerInventory || ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	FPDMarketEntry* Entry = FindEntry(ItemID);
	if (!Entry || (Entry->Stock >= 0 && Entry->Stock < Quantity))
	{
		return false;
	}

	const int32 UnitPrice = FMath::Max(0, Entry->OverridePrice >= 0 ? Entry->OverridePrice : Entry->ItemData.Price);
	const int32 TotalPrice = UnitPrice * Quantity;

	if (!BuyerInventory->SpendGold(TotalPrice))
	{
		return false;
	}

	if (!BuyerInventory->AddItem(Entry->ItemData, Quantity))
	{
		BuyerInventory->AddGold(TotalPrice);
		return false;
	}

	if (Entry->Stock >= 0)
	{
		Entry->Stock -= Quantity;
	}

	return true;
}

bool UPDMarketComponent::SellItem(UPDInventoryComponent* SellerInventory, FName ItemID, int32 Quantity)
{
	if (!SellerInventory || ItemID.IsNone() || Quantity <= 0 || !SellerInventory->HasItem(ItemID, Quantity))
	{
		return false;
	}

	FPDMarketEntry* Entry = FindEntry(ItemID);
	if (!Entry)
	{
		return false;
	}

	const int32 UnitPrice = FMath::Max(0, Entry->OverridePrice >= 0 ? Entry->OverridePrice : Entry->ItemData.Price);

	if (!SellerInventory->RemoveItem(ItemID, Quantity))
	{
		return false;
	}

	SellerInventory->AddGold(UnitPrice * Quantity);

	if (Entry->Stock >= 0)
	{
		Entry->Stock += Quantity;
	}

	return true;
}

FPDMarketEntry* UPDMarketComponent::FindEntry(FName ItemID)
{
	for (FPDMarketEntry& Entry : Goods)
	{
		if (Entry.ItemData.ItemID == ItemID)
		{
			return &Entry;
		}
	}

	return nullptr;
}
