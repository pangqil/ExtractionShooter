#include "Items/PDQuickSlotComponent.h"

#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDStashComponent.h"

#include "GameFramework/Actor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

UPDQuickSlotComponent::UPDQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuickSlots();

	if (UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDQuickSlotComponent::HandleInventoryChanged);
		SyncQuickSlotsWithInventory();
	}
}

void UPDQuickSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPDInventoryComponent* InventoryComponent = FindOwnerInventory())
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDQuickSlotComponent::HandleInventoryChanged);
	}

	Super::EndPlay(EndPlayReason);
}

UPDInventoryComponent* UPDQuickSlotComponent::FindOwnerInventory() const
{
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
}

int32 UPDQuickSlotComponent::GetInventoryItemQuantity(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	const UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent)
	{
		return 0;
	}

	int32 Quantity = 0;
	for (const FPDInventorySlot& Slot : InventoryComponent->Items)
	{
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemID)
		{
			Quantity += Slot.Quantity;
		}
	}

	return Quantity;
}

bool UPDQuickSlotComponent::SyncQuickSlotsWithInventory()
{
	bool bChanged = false;

	for (FPDInventorySlot& Slot : QuickSlotItems)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		const int32 InventoryQuantity = GetInventoryItemQuantity(Slot.ItemData.ItemID);
		if (InventoryQuantity <= 0)
		{
			Slot.Clear();
			bChanged = true;
			continue;
		}

		if (Slot.Quantity != InventoryQuantity)
		{
			Slot.Quantity = InventoryQuantity;
			Slot.bIsEmpty = false;
			bChanged = true;
		}
	}

	return bChanged;
}

void UPDQuickSlotComponent::HandleInventoryChanged()
{
	if (SyncQuickSlotsWithInventory())
	{
		OnQuickSlotsChanged.Broadcast();
	}
}

int32 UPDQuickSlotComponent::FindEmptySlot() const
{
	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (QuickSlotItems[Index].IsEmpty())
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UPDQuickSlotComponent::InitializeQuickSlots()
{
	const int32 MaxSlotCount = GetMaxSlotCount();

	if (MaxSlotCount <= 0)
	{
		QuickSlotItems.Empty();
		OnQuickSlotsChanged.Broadcast();
		return;
	}

	const int32 OldCount = QuickSlotItems.Num();

	QuickSlotItems.SetNum(MaxSlotCount);

	for (int32 Index = OldCount; Index < QuickSlotItems.Num(); ++Index)
	{
		QuickSlotItems[Index].Clear();
	}

	for (FPDInventorySlot& Slot : QuickSlotItems)
	{
		if (Slot.IsEmpty())
		{
			Slot.Clear();
		}
	}

	OnQuickSlotsChanged.Broadcast();
}

void UPDQuickSlotComponent::ResetQuickSlots()
{
	QuickSlotItems.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : QuickSlotItems)
	{
		Slot.Clear();
	}

	OnQuickSlotsChanged.Broadcast();
}

int32 UPDQuickSlotComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0 || ItemData.ItemType != EPDItemType::Consumable)
	{
		return 0;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	int32 RemainingQuantity = Quantity;
	int32 AddedQuantity = 0;

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : QuickSlotItems)
		{
			if (!Slot.IsEmpty() && Slot.ItemData.ItemID == ItemData.ItemID && Slot.Quantity < MaxStack)
			{
				const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack - Slot.Quantity);
				Slot.Quantity += AddAmount;
				RemainingQuantity -= AddAmount;
				AddedQuantity += AddAmount;

				if (RemainingQuantity <= 0)
				{
					OnQuickSlotsChanged.Broadcast();
					return AddedQuantity;
				}
			}
		}
	}

	while (RemainingQuantity > 0)
	{
		const int32 EmptySlot = FindEmptySlot();

		if (EmptySlot == INDEX_NONE)
		{
			break;
		}

		const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack);

		QuickSlotItems[EmptySlot].ItemData = ItemData;
		QuickSlotItems[EmptySlot].Quantity = AddAmount;
		QuickSlotItems[EmptySlot].bIsEmpty = false;

		RemainingQuantity -= AddAmount;
		AddedQuantity += AddAmount;
	}

	if (AddedQuantity > 0)
	{
		OnQuickSlotsChanged.Broadcast();
	}

	return AddedQuantity;
}

int32 UPDQuickSlotComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (ItemData.ItemID.IsNone() || Quantity <= 0 || ItemData.ItemType != EPDItemType::Consumable)
	{
		return 0;
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(QuickSlotItems[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		OnQuickSlotsChanged.Broadcast();
	}

	return AddedQuantity;
}

bool UPDQuickSlotComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex)
	{
		return false;
	}

	if (QuickSlotItems[SourceSlotIndex].IsEmpty())
	{
		return false;
	}

	Swap(QuickSlotItems[SourceSlotIndex], QuickSlotItems[TargetSlotIndex]);
	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::StoreInventorySlotQuantityToSlot(UPDInventoryComponent* SourceInventory, int32 SourceSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	if (!SourceInventory || SourceInventory->Items.Num() != SourceInventory->GetMaxSlotCount())
	{
		if (SourceInventory)
		{
			SourceInventory->InitializeInventory();
		}
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!SourceInventory || !SourceInventory->Items.IsValidIndex(SourceSlotIndex) || !QuickSlotItems.IsValidIndex(TargetQuickSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SourceInventory->Items[SourceSlotIndex];
	if (SourceSlot.IsEmpty() || SourceSlot.ItemData.ItemType != EPDItemType::Consumable)
	{
		return false;
	}

	int32 ExistingSlotIndex = INDEX_NONE;
	for (int32 Index = 0; Index < QuickSlotItems.Num(); ++Index)
	{
		if (Index != TargetQuickSlotIndex && !QuickSlotItems[Index].IsEmpty() && QuickSlotItems[Index].ItemData.ItemID == SourceSlot.ItemData.ItemID)
		{
			ExistingSlotIndex = Index;
			break;
		}
	}

	if (ExistingSlotIndex != INDEX_NONE)
	{
		Swap(QuickSlotItems[ExistingSlotIndex], QuickSlotItems[TargetQuickSlotIndex]);
	}
	else
	{
		FPDInventorySlot& TargetSlot = QuickSlotItems[TargetQuickSlotIndex];
		TargetSlot.ItemData = SourceSlot.ItemData;
		TargetSlot.Quantity = GetInventoryItemQuantity(SourceSlot.ItemData.ItemID);
		if (TargetSlot.Quantity <= 0)
		{
			TargetSlot.Quantity = SourceSlot.Quantity;
		}
		TargetSlot.bIsEmpty = false;
	}

	SyncQuickSlotsWithInventory();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::StoreStashSlotQuantityToSlot(UPDStashComponent* SourceStash, int32 SourceStashSlotIndex, int32 TargetQuickSlotIndex, int32 Quantity)
{
	if (!SourceStash || SourceStash->StashItems.Num() != SourceStash->GetMaxSlotCount())
	{
		if (SourceStash)
		{
			SourceStash->InitializeStash();
		}
	}

	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!SourceStash || !SourceStash->StashItems.IsValidIndex(SourceStashSlotIndex) || !QuickSlotItems.IsValidIndex(TargetQuickSlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const FPDInventorySlot& SourceSlot = SourceStash->StashItems[SourceStashSlotIndex];
	if (SourceSlot.IsEmpty() || SourceSlot.ItemData.ItemType != EPDItemType::Consumable)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(SourceStash->StashItems[SourceStashSlotIndex], QuickSlotItems[TargetQuickSlotIndex], Quantity);
	if (bMoved)
	{
		SourceStash->OnStashChanged.Broadcast();
		OnQuickSlotsChanged.Broadcast();
	}

	return bMoved;
}

bool UPDQuickSlotComponent::TakeQuickSlotQuantityToInventorySlot(UPDInventoryComponent* TargetInventory, int32 QuickSlotIndex, int32 TargetInventorySlotIndex, int32 Quantity)
{
	return RemoveItemFromSlot(QuickSlotIndex, Quantity);
}

bool UPDQuickSlotComponent::TakeQuickSlotQuantityToStashSlot(UPDStashComponent* TargetStash, int32 QuickSlotIndex, int32 TargetStashSlotIndex, int32 Quantity)
{
	return RemoveItemFromSlot(QuickSlotIndex, Quantity);
}

bool UPDQuickSlotComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (QuickSlotItems[SlotIndex].IsEmpty())
	{
		return false;
	}

	QuickSlotItems[SlotIndex].Clear();
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UPDQuickSlotComponent::UseQuickSlot(int32 SlotIndex)
{
	if (QuickSlotItems.Num() != GetMaxSlotCount())
	{
		InitializeQuickSlots();
	}

	if (!QuickSlotItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& Slot = QuickSlotItems[SlotIndex];
	if (Slot.IsEmpty() || Slot.ItemData.ItemType != EPDItemType::Consumable)
	{
		return false;
	}

	UPDInventoryComponent* InventoryComponent = FindOwnerInventory();
	if (!InventoryComponent || !InventoryComponent->HasItem(Slot.ItemData.ItemID, 1))
	{
		SyncQuickSlotsWithInventory();
		OnQuickSlotsChanged.Broadcast();
		return false;
	}

	const bool bRemoved=InventoryComponent->RemoveItem(Slot.ItemData.ItemID, 1);
	if (!bRemoved) return false;

	if (Slot.ItemData.UseEffect)
	{
		UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
		if (ASC)
		{
			FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
			Context.AddSourceObject(GetOwner());
			ASC->ApplyGameplayEffectToSelf(Slot.ItemData.UseEffect->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
		}
	}

	return true;
}

bool UPDQuickSlotComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 FoundQuantity = 0;

	for (const FPDInventorySlot& Slot : QuickSlotItems)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

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
