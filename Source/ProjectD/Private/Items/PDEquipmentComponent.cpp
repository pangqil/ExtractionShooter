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

	// Legacy safety: old weapon rows may have EquipmentSlotType=None and WeaponType set.
	// New data table rows should explicitly use EquipmentSlotType=Weapon.
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

	if (!ApplyCharacterEquipSideEffects(InventorySlot.ItemData))
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
	const int32 AddedQuantity = InventoryComponent->AddItemPartial(ItemToReturn.ItemData, ItemToReturn.Quantity);
	if (AddedQuantity != ItemToReturn.Quantity)
	{
		// 공간이 부족하면 장착 상태를 유지한다. AddItemPartial이 부분 추가한 경우는 되돌릴 수 없으므로
		// 장비는 해제하지 않는다. 장비 아이템 MaxStack은 1로 두는 것을 권장한다.
		return false;
	}

	RemoveCharacterEquipSideEffects(ItemToReturn.ItemData);
	EquippedItem->ItemSlot.Clear();
	BroadcastSlotChanged(SlotType);
	return true;
}

bool UPDEquipmentComponent::ApplyCharacterEquipSideEffects(const FPDItemData& ItemData) const
{
	// APDWeaponBase 직접 참조를 피하기 위해 WeaponClass를 여기서 검사하지 않는다.
	// 무기 여부는 데이터의 WeaponType으로만 판단하고, 실제 WeaponClass 유효성은
	// APDPlayerCharacter::TryAutoEquipWeaponItem 내부에서 처리한다.
	if (ItemData.WeaponType == EWeaponType::None)
	{
		// 방어구/가방 등은 현재 데이터 저장까지만 처리. 능력치/GAS 효과는 추후 연동.
		return true;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		return PlayerCharacter->TryAutoEquipWeaponItem(ItemData);
	}

	return false;
}

void UPDEquipmentComponent::RemoveCharacterEquipSideEffects(const FPDItemData& ItemData) const
{
	if (ItemData.WeaponType == EWeaponType::None)
	{
		return;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner()))
	{
		PlayerCharacter->RemoveEquippedWeaponItem(ItemData);
	}
}

void UPDEquipmentComponent::BroadcastSlotChanged(EPDEquipmentSlotType SlotType)
{
	const FPDInventorySlot EquippedSlot = GetEquippedSlot(SlotType);
	OnEquipmentSlotChanged.Broadcast(SlotType, EquippedSlot);
	OnEquipmentChanged.Broadcast();
}
