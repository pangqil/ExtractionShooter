#include "Items/PDEquipmentComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Core/PDPlayerState.h"
#include "Items/PDInventoryComponent.h"
#include "Net/UnrealNetwork.h"

UPDEquipmentComponent::UPDEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InitializeDefaultSlots();
	}
}

void UPDEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDEquipmentComponent, ReplicatedEquippedItems);
}

void UPDEquipmentComponent::OnRep_EquippedItems()
{
	RebuildEquippedItemsFromReplication();
	OnEquipmentChanged.Broadcast();
}

void UPDEquipmentComponent::ServerEquipItemFromInventory_Implementation(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex)
{
	EquipItemFromInventory(InventoryComponent, InventorySlotIndex);
}

void UPDEquipmentComponent::ServerEquipItemFromInventoryToSlot_Implementation(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType)
{
	EquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, TargetSlotType);
}

void UPDEquipmentComponent::ServerUnequipItemToInventory_Implementation(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType)
{
	UnequipItemToInventory(InventoryComponent, SlotType);
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
	SyncReplicatedEquippedItems();
}

void UPDEquipmentComponent::RebuildEquippedItemsFromReplication()
{
	EquippedItems.Empty();
	for (const FPDEquippedItem& EquippedItem : ReplicatedEquippedItems)
	{
		if (EquippedItem.SlotType != EPDEquipmentSlotType::None)
		{
			EquippedItems.Add(EquippedItem.SlotType, EquippedItem);
		}
	}
}

void UPDEquipmentComponent::SyncReplicatedEquippedItems()
{
	ReplicatedEquippedItems.Reset();
	for (const TPair<EPDEquipmentSlotType, FPDEquippedItem>& Pair : EquippedItems)
	{
		ReplicatedEquippedItems.Add(Pair.Value);
	}
}

EPDEquipmentSlotType UPDEquipmentComponent::ResolveEquipmentSlotType(const FPDItemData& ItemData) const
{
	if (ItemData.WeaponType != EWeaponType::None)
	{
		return EPDEquipmentSlotType::Weapon;
	}

	if (ItemData.ItemType != EPDItemType::Equipment)
	{
		return EPDEquipmentSlotType::None;
	}

	if (ItemData.EquipmentSlotType != EPDEquipmentSlotType::None)
	{
		return ItemData.EquipmentSlotType;
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
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerEquipItemFromInventory(InventoryComponent, InventorySlotIndex);
		return true;
	}

	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		return false;
	}

	const FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty())
	{
		return false;
	}

	return EquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, ResolveEquipmentSlotType(InventorySlot.ItemData));
}

bool UPDEquipmentComponent::EquipItemFromInventoryToSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDEquipmentSlotType TargetSlotType)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerEquipItemFromInventoryToSlot(InventoryComponent, InventorySlotIndex, TargetSlotType);
		return true;
	}

	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(InventorySlotIndex) || TargetSlotType == EPDEquipmentSlotType::None)
	{
		return false;
	}

	const FPDInventorySlot InventorySlot = InventoryComponent->Items[InventorySlotIndex];
	if (InventorySlot.IsEmpty() ||
		(InventorySlot.ItemData.ItemType != EPDItemType::Equipment && InventorySlot.ItemData.WeaponType == EWeaponType::None) ||
		InventorySlot.Quantity != 1)
	{
		return false;
	}

	if (ResolveEquipmentSlotType(InventorySlot.ItemData) != TargetSlotType)
	{
		return false;
	}

	FPDEquippedItem* CurrentEquippedItem = EquippedItems.Find(TargetSlotType);
	FPDInventorySlot PreviousEquippedSlot;
	const bool bHadPreviousItem = CurrentEquippedItem && !CurrentEquippedItem->ItemSlot.IsEmpty();
	if (bHadPreviousItem)
	{
		PreviousEquippedSlot = CurrentEquippedItem->ItemSlot;
	}

	const float NewBagCapacityWeight = TargetSlotType == EPDEquipmentSlotType::Bag ? FMath::Max(0.f, InventorySlot.ItemData.BagCapacityWeight) : [&]()
	{
		const FPDInventorySlot CurrentBagSlot = GetEquippedSlot(EPDEquipmentSlotType::Bag);
		return CurrentBagSlot.IsEmpty() ? 0.f : FMath::Max(0.f, CurrentBagSlot.ItemData.BagCapacityWeight);
	}();

	if (!InventoryComponent->CanFitWeightAfterEquipmentChange(InventorySlot, PreviousEquippedSlot, NewBagCapacityWeight))
	{
		InventoryComponent->BroadcastWeightLimitExceeded();
		return false;
	}

	if (bHadPreviousItem)
	{
		RemoveCharacterEquipSideEffects(PreviousEquippedSlot);
	}

	if (!ApplyCharacterEquipSideEffects(InventorySlot))
	{
		if (bHadPreviousItem)
		{
			ApplyCharacterEquipSideEffects(PreviousEquippedSlot);
		}
		return false;
	}

	FPDInventorySlot EquippedSlot = InventorySlot;
	EquippedSlot.Quantity = 1;
	EquippedSlot.bIsEmpty = false;

	FPDEquippedItem NewEquippedItem;
	NewEquippedItem.SlotType = TargetSlotType;
	NewEquippedItem.ItemSlot = EquippedSlot;
	EquippedItems.Add(TargetSlotType, NewEquippedItem);

	if (bHadPreviousItem)
	{
		InventoryComponent->Items[InventorySlotIndex] = PreviousEquippedSlot;
		InventoryComponent->Items[InventorySlotIndex].Quantity = FMath::Max(1, InventoryComponent->Items[InventorySlotIndex].Quantity);
		InventoryComponent->Items[InventorySlotIndex].bIsEmpty = false;
	}
	else
	{
		InventoryComponent->Items[InventorySlotIndex].Clear();
	}

	SyncReplicatedEquippedItems();
	InventoryComponent->OnInventoryChanged.Broadcast();
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, EquippedSlot);
	return true;
}


bool UPDEquipmentComponent::TryEquipNewItem(const FPDItemData& ItemData)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return false;
	}

	const EPDEquipmentSlotType TargetSlotType = ResolveEquipmentSlotType(ItemData);
	if (TargetSlotType == EPDEquipmentSlotType::None || IsSlotOccupied(TargetSlotType))
	{
		return false;
	}

	FPDInventorySlot NewSlot;
	NewSlot.ItemData = ItemData;
	NewSlot.Quantity = 1;
	NewSlot.bIsEmpty = false;
	NewSlot.ModificationLevel = 0;

	if (!ApplyCharacterEquipSideEffects(NewSlot))
	{
		return false;
	}

	FPDEquippedItem NewEquippedItem;
	NewEquippedItem.SlotType = TargetSlotType;
	NewEquippedItem.ItemSlot = NewSlot;
	EquippedItems.Add(TargetSlotType, NewEquippedItem);

	SyncReplicatedEquippedItems();
	BroadcastSlotChanged(TargetSlotType);
	BroadcastModificationApplied(TargetSlotType, NewSlot);
	return true;
}

bool UPDEquipmentComponent::UnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerUnequipItemToInventory(InventoryComponent, SlotType);
		return true;
	}

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
	const float NewBagCapacityWeight = SlotType == EPDEquipmentSlotType::Bag ? 0.f : [&]()
	{
		const FPDInventorySlot CurrentBagSlot = GetEquippedSlot(EPDEquipmentSlotType::Bag);
		return CurrentBagSlot.IsEmpty() ? 0.f : FMath::Max(0.f, CurrentBagSlot.ItemData.BagCapacityWeight);
	}();

	if (!InventoryComponent->CanFitWeightAfterEquipmentChange(FPDInventorySlot(), ItemToReturn, NewBagCapacityWeight))
	{
		InventoryComponent->BroadcastWeightLimitExceeded();
		return false;
	}

	const int32 AddedQuantity = InventoryComponent->AddSlotPartial(ItemToReturn);
	if (AddedQuantity != ItemToReturn.Quantity)
	{
		return false;
	}

	RemoveCharacterEquipSideEffects(ItemToReturn);
	EquippedItem->ItemSlot.Clear();
	SyncReplicatedEquippedItems();
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

	APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner());
	if (!PlayerCharacter)
	{
		if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(GetOwner()))
		{
			PlayerCharacter = PDPlayerState->GetPDPlayerCharacter();
		}
	}

	if (PlayerCharacter)
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

	APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwner());
	if (!PlayerCharacter)
	{
		if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(GetOwner()))
		{
			PlayerCharacter = PDPlayerState->GetPDPlayerCharacter();
		}
	}

	if (PlayerCharacter)
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
