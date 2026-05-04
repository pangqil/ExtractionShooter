#include "Items/PDStashComponent.h"

UPDStashComponent::UPDStashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDStashComponent::StoreItem(const FPDItemData& ItemData, int32 Quantity, UPDInventoryComponent* SourceInventory)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	if (SourceInventory)
	{
		if (!SourceInventory->HasItem(ItemData.ItemID, Quantity))
		{
			return false;
		}

		if (!SourceInventory->RemoveItem(ItemData.ItemID, Quantity))
		{
			return false;
		}
	}

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : StashItems)
		{
			if (Slot.ItemData.ItemID == ItemData.ItemID && Slot.Quantity < MaxStack)
			{
				const int32 AddAmount = FMath::Min(Quantity, MaxStack - Slot.Quantity);
				Slot.Quantity += AddAmount;
				Quantity -= AddAmount;
				if (Quantity <= 0)
				{
					return true;
				}
			}
		}
	}

	while (Quantity > 0)
	{
		FPDInventorySlot NewSlot;
		NewSlot.ItemData = ItemData;
		NewSlot.Quantity = FMath::Min(Quantity, MaxStack);
		StashItems.Add(NewSlot);
		Quantity -= NewSlot.Quantity;
	}

	return true;
}

bool UPDStashComponent::TakeItem(FName ItemID, int32 Quantity, UPDInventoryComponent* TargetInventory)
{
	const int32 RequestedQuantity = Quantity;
	if (ItemID.IsNone() || Quantity <= 0 || !HasItem(ItemID, Quantity))
	{
		return false;
	}

	FPDItemData ItemData;
	for (const FPDInventorySlot& Slot : StashItems)
	{
		if (Slot.ItemData.ItemID == ItemID)
		{
			ItemData = Slot.ItemData;
			break;
		}
	}

	for (int32 Index = StashItems.Num() - 1; Index >= 0 && Quantity > 0; --Index)
	{
		FPDInventorySlot& Slot = StashItems[Index];
		if (Slot.ItemData.ItemID != ItemID)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Quantity, Slot.Quantity);
		Slot.Quantity -= RemoveAmount;
		Quantity -= RemoveAmount;
		if (Slot.Quantity <= 0)
		{
			StashItems.RemoveAt(Index);
		}
	}

	if (TargetInventory)
	{
		TargetInventory->AddItem(ItemData, RequestedQuantity);
	}

	return true;
}

bool UPDStashComponent::HasItem(FName ItemID, int32 Quantity) const
{
	int32 FoundQuantity = 0;
	for (const FPDInventorySlot& Slot : StashItems)
	{
		if (Slot.ItemData.ItemID == ItemID)
		{
			FoundQuantity += Slot.Quantity;
			if (FoundQuantity >= Quantity)
			{
				return true;
			}
		}
	}
	return false;
}
