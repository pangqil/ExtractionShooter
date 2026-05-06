#include "Items/PDInventoryComponent.h"

UPDInventoryComponent::UPDInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDInventoryComponent::AddItem(const FPDItemData& ItemData, int32 Quantity)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : Items)
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
		Items.Add(NewSlot);
		Quantity -= NewSlot.Quantity;
	}

	return true;
}

bool UPDInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0 || !HasItem(ItemID, Quantity))
	{
		return false;
	}

	for (int32 Index = Items.Num() - 1; Index >= 0 && Quantity > 0; --Index)
	{
		FPDInventorySlot& Slot = Items[Index];
		if (Slot.ItemData.ItemID != ItemID)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Quantity, Slot.Quantity);
		Slot.Quantity -= RemoveAmount;
		Quantity -= RemoveAmount;

		if (Slot.Quantity <= 0)
		{
			Items.RemoveAt(Index);
		}
	}

	return true;
}

bool UPDInventoryComponent::UseItem(FName ItemID, int32 Quantity)
{
	return RemoveItem(ItemID, Quantity);
}

bool UPDInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 FoundQuantity = 0;
	for (const FPDInventorySlot& Slot : Items)
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

void UPDInventoryComponent::AddGold(int32 Amount)
{
	if (Amount > 0)
	{
		Gold += Amount;
	}
}

bool UPDInventoryComponent::SpendGold(int32 Amount)
{
	if (Amount < 0 || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	return true;
}
