#include "Items/PDEquipmentComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Items/PDInventoryComponent.h"

UPDEquipmentComponent::UPDEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultSlots();
}

void UPDEquipmentComponent::InitializeDefaultSlots()
{
	const EPDEquipmentSlotType DefaultSlots[] =
	{
		EPDEquipmentSlotType::Weapon,
		EPDEquipmentSlotType::Head,
		EPDEquipmentSlotType::Armor,
		EPDEquipmentSlotType::Bag,
	};

	for (EPDEquipmentSlotType SlotType : DefaultSlots)
	{
		if (!EquippedItems.Contains(SlotType))
		{
			FPDEquippedItem EmptyItem;
			EmptyItem.SlotType = SlotType;
			EquippedItems.Add(SlotType, EmptyItem);
		}
	}
}

EPDEquipmentSlotType UPDEquipmentComponent::ResolveEquipmentSlotType(const FPDItemData& ItemData) const
{
	if (ItemData.ItemType != EPDItemType::Equipment)
	{
		return EPDEquipmentSlotType::None;
	}

	if (ItemData.EquipmentSlotType != EPDEquipmentSlotType::None)
	{
		return ItemData.EquipmentSlotType;
	}

	if (ItemData.WeaponType != EWeaponType::None)
	{
		return EPDEquipmentSlotType::Weapon;
	}

	return EPDEquipmentSlotType::None;
}

bool UPDEquipmentComponent::CanEquipItem(const FPDItemData& ItemData) const
{
	return ResolveEquipmentSlotType(ItemData) != EPDEquipmentSlotType::None;
}

bool UPDEquipmentComponent::IsSlotOccupied(EPDEquipmentSlotType SlotType) const
{
	const FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType);
	return EquippedItem && !EquippedItem->ItemSlot.IsEmpty();
}

FPDInventorySlot UPDEquipmentComponent::GetEquippedSlot(EPDEquipmentSlotType SlotType) const
{
	if (const FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType))
	{
		return EquippedItem->ItemSlot;
	}

	return FPDInventorySlot();
}

bool UPDEquipmentComponent::EquipItemFromInventory(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex)
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty() || InventorySlot.ItemData.ItemType != EPDItemType::Equipment)
	{
		return false;
	}

	const EPDEquipmentSlotType TargetSlotType = ResolveEquipmentSlotType(InventorySlot.ItemData);
	if (TargetSlotType == EPDEquipmentSlotType::None || IsSlotOccupied(TargetSlotType))
	{
		return false;
	}

	if (!ApplyCharacterEquipSideEffects(InventorySlot))
	{
		return false;
	}

	FPDInventorySlot EquippedSlot = InventorySlot;
	EquippedSlot.Quantity = 1;
	EquippedSlot.bIsEmpty = false;

	FPDEquippedItem EquippedItem;
	EquippedItem.SlotType = TargetSlotType;
	EquippedItem.ItemSlot = EquippedSlot;
	EquippedItems.Add(TargetSlotType, EquippedItem);

	InventoryComponent->RemoveItemFromSlot(InventorySlotIndex, 1);
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, EquippedSlot);
	return true;
}

bool UPDEquipmentComponent::UnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType)
{
	if (!InventoryComponent || SlotType == EPDEquipmentSlotType::None)
	{
		return false;
	}

	FPDEquippedItem* EquippedItem = EquippedItems.Find(SlotType);
	if (!EquippedItem || EquippedItem->ItemSlot.IsEmpty())
	{
		return false;
	}

	const FPDInventorySlot ItemToReturn = EquippedItem->ItemSlot;
	const int32 AddedQuantity = InventoryComponent->AddSlotPartial(ItemToReturn);
	if (AddedQuantity != ItemToReturn.Quantity)
	{
		return false;
	}

	RemoveCharacterEquipSideEffects(ItemToReturn);
	EquippedItem->ItemSlot.Clear();
	BroadcastSlotChanged(SlotType);
	return true;
}

bool UPDEquipmentComponent::ApplyCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot) const
{
	const FPDItemData& ItemData = ItemSlot.ItemData;

	if (ItemData.WeaponType == EWeaponType::None)
	{
		return true;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		return PlayerCharacter->TryAutoEquipWeaponSlot(ItemSlot);
	}

	return false;
}

void UPDEquipmentComponent::RemoveCharacterEquipSideEffects(const FPDInventorySlot& ItemSlot) const
{
	const FPDItemData& ItemData = ItemSlot.ItemData;
	if (ItemData.WeaponType == EWeaponType::None)
	{
		return;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		PlayerCharacter->RemoveEquippedWeaponItem(ItemData);
	}
}

int32 UPDEquipmentComponent::ConvertModificationLevelToGasLevel(int32 ModificationLevel) const
{
	return FMath::Max(1, ModificationLevel + 1);
}

void UPDEquipmentComponent::BroadcastModificationApplied(EPDEquipmentSlotType SlotType, const FPDInventorySlot& EquippedSlot)
{
	const int32 GasLevel = ConvertModificationLevelToGasLevel(EquippedSlot.ModificationLevel);
	OnEquipmentModificationApplied.Broadcast(SlotType, EquippedSlot, GasLevel);
}

void UPDEquipmentComponent::BroadcastSlotChanged(EPDEquipmentSlotType SlotType)
{
	const FPDInventorySlot EquippedSlot = GetEquippedSlot(SlotType);
	OnEquipmentSlotChanged.Broadcast(SlotType, EquippedSlot);
	OnEquipmentChanged.Broadcast();
}
