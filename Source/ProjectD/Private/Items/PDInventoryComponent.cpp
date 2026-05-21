#include "Items/PDInventoryComponent.h"

#include "Core/PDPlayerState.h"
#include "Data/PDQuestComponent.h"
#include "Characters/PDPlayerCharacter.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDEquipmentComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	UPDQuestComponent* FindOwnerQuestComponent(const UActorComponent* Component)
	{
		if (!Component || !Component->GetOwner())
		{
			return nullptr;
		}

		if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(Component->GetOwner()))
		{
			return PDPlayerState->GetQuestComponent();
		}

		return Component->GetOwner()->FindComponentByClass<UPDQuestComponent>();
	}

	UPDEquipmentComponent* FindOwnerEquipmentComponent(const UActorComponent* Component)
	{
		if (!Component || !Component->GetOwner())
		{
			return nullptr;
		}

		if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(Component->GetOwner()))
		{
			return PDPlayerState->GetEquipmentComponent();
		}

		return Component->GetOwner()->FindComponentByClass<UPDEquipmentComponent>();
	}

	APDPlayerCharacter* FindOwnerPlayerCharacter(const UActorComponent* Component)
	{
		if (!Component || !Component->GetOwner())
		{
			return nullptr;
		}

		if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(Component->GetOwner()))
		{
			return PlayerCharacter;
		}

		if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(Component->GetOwner()))
		{
			return PDPlayerState->GetPDPlayerCharacter();
		}

		return nullptr;
	}
}

UPDInventoryComponent::UPDInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeInventory();
}

void UPDInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDInventoryComponent, Items);
	DOREPLIFETIME(UPDInventoryComponent, Gold);
}

void UPDInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::OnRep_Gold()
{
	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::ServerRemoveItemFromSlot_Implementation(int32 SlotIndex, int32 Quantity)
{
	RemoveItemFromSlot(SlotIndex, Quantity);
}

void UPDInventoryComponent::ServerDropItemFromSlot_Implementation(int32 SlotIndex, int32 Quantity)
{
	DropItemFromSlot(SlotIndex, Quantity);
}

void UPDInventoryComponent::ServerUseItemFromSlot_Implementation(int32 SlotIndex)
{
	UseItemFromSlot(SlotIndex);
}

void UPDInventoryComponent::ServerMoveSlotQuantityToSlot_Implementation(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
}

bool UPDInventoryComponent::AddItem(const FPDItemData& ItemData, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	return AddItemPartial(ItemData, Quantity) > 0;
}

bool UPDInventoryComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!ItemDataTable || ItemID.IsNone()) return false;

	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("AddItemByID"), Rows);

	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
			return AddItem(*Row, Quantity);
	}

	return false;
}

bool UPDInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (ItemID.IsNone() || Quantity <= 0 || !HasItem(ItemID, Quantity))
	{
		return false;
	}

	for (int32 Index = Items.Num() - 1; Index >= 0 && Quantity > 0; --Index)
	{
		FPDInventorySlot& Slot = Items[Index];

		if (Slot.IsEmpty() || Slot.ItemData.ItemID != ItemID)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Quantity, Slot.Quantity);
		Slot.Quantity -= RemoveAmount;
		Quantity -= RemoveAmount;

		if (Slot.Quantity <= 0)
		{
			Slot.Clear();
		}
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UPDInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerRemoveItemFromSlot(SlotIndex, Quantity);
		return true;
	}
	if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
	{
		return false;
	}

	FPDInventorySlot& Slot = Items[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const int32 RemoveAmount = FMath::Min(Quantity, Slot.Quantity);
	Slot.Quantity -= RemoveAmount;

	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UPDInventoryComponent::DropItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerDropItemFromSlot(SlotIndex, Quantity);
		return true;
	}
	if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
	{
		return false;
	}

	const FPDInventorySlot& Slot = Items[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const FName ItemID = Slot.ItemData.ItemID;
	const int32 DropAmount = FMath::Min(Quantity, Slot.Quantity);
	const bool bDropped = RemoveItemFromSlot(SlotIndex, Quantity);

	if (bDropped)
	{
		if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
		{
			QuestComponent->ReportItemDropped(ItemID, DropAmount);
		}
	}

	return bDropped;
}

bool UPDInventoryComponent::UseItemFromSlot(int32 SlotIndex)
{
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerUseItemFromSlot(SlotIndex);
		return true;
	}
	if (!Items.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& Slot = Items[SlotIndex];
	if (Slot.IsEmpty() || Slot.ItemData.ItemType != EPDItemType::Consumable)
	{
		return false;
	}

	return RemoveItemFromSlot(SlotIndex, 1);
}

bool UPDInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 TotalQuantity = 0;

	for (const FPDInventorySlot& Slot : Items)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		if (Slot.ItemData.ItemID == ItemID)
		{
			TotalQuantity += Slot.Quantity;

			if (TotalQuantity >= Quantity)
			{
				return true;
			}
		}
	}

	return false;
}

void UPDInventoryComponent::AddGold(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (Amount > 0)
	{
		Gold += Amount;
		OnInventoryChanged.Broadcast();
	}
}

bool UPDInventoryComponent::SpendGold(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (Amount <= 0 || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	OnInventoryChanged.Broadcast();
	return true;
}

float UPDInventoryComponent::GetCurrentWeight() const
{
	float TotalWeight = 0.f;

	for (const FPDInventorySlot& Slot : Items)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		TotalWeight += FMath::Max(0.f, Slot.ItemData.Weight) * FMath::Max(0, Slot.Quantity);
	}

	return TotalWeight;
}

float UPDInventoryComponent::GetMaxWeight() const
{
	float MaxWeight = FMath::Max(0.f, BaseCarryWeight);

	if (const UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipmentComponent(this))
	{
		const FPDInventorySlot EquippedBagSlot = EquipmentComponent->GetEquippedSlot(EPDEquipmentSlotType::Bag);
		if (!EquippedBagSlot.IsEmpty())
		{
			MaxWeight += FMath::Max(0.f, EquippedBagSlot.ItemData.BagCapacityWeight);
		}
	}

	return MaxWeight;
}

bool UPDInventoryComponent::CanAddWeight(const FPDItemData& ItemData, int32 Quantity) const
{
	if (Quantity <= 0)
	{
		return true;
	}

	const float AddWeight = FMath::Max(0.f, ItemData.Weight) * Quantity;
	return GetCurrentWeight() + AddWeight <= GetMaxWeight() + KINDA_SMALL_NUMBER;
}

bool UPDInventoryComponent::CanAddSlotWeight(const FPDInventorySlot& SourceSlot, int32 Quantity) const
{
	if (SourceSlot.IsEmpty() || Quantity <= 0)
	{
		return true;
	}

	const int32 ClampedQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	return CanAddWeight(SourceSlot.ItemData, ClampedQuantity);
}

bool UPDInventoryComponent::CanFitWeightAfterEquipmentChange(const FPDInventorySlot& ItemLeavingInventory, const FPDInventorySlot& ItemEnteringInventory, float NewBagCapacityWeight) const
{
	float ExpectedWeight = GetCurrentWeight();

	if (!ItemLeavingInventory.IsEmpty())
	{
		ExpectedWeight -= FMath::Max(0.f, ItemLeavingInventory.ItemData.Weight) * FMath::Max(0, ItemLeavingInventory.Quantity);
	}

	if (!ItemEnteringInventory.IsEmpty())
	{
		ExpectedWeight += FMath::Max(0.f, ItemEnteringInventory.ItemData.Weight) * FMath::Max(0, ItemEnteringInventory.Quantity);
	}

	const float ExpectedMaxWeight = FMath::Max(0.f, BaseCarryWeight) + FMath::Max(0.f, NewBagCapacityWeight);
	return ExpectedWeight <= ExpectedMaxWeight + KINDA_SMALL_NUMBER;
}

void UPDInventoryComponent::BroadcastInventoryMessage(const FText& Message)
{
	if (!Message.IsEmpty())
	{
		OnInventoryMessage.Broadcast(Message);
	}
}

void UPDInventoryComponent::BroadcastWeightLimitExceeded()
{
	OnInventoryWeightLimitExceeded.Broadcast(GetCurrentWeight(), GetMaxWeight());
	BroadcastInventoryMessage(FText::FromString(TEXT("무게가 초과되었습니다.")));
}

int32 UPDInventoryComponent::FindEmptySlot() const
{
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i].IsEmpty())
		{
			return i;
		}
	}

	return INDEX_NONE;
}


int32 UPDInventoryComponent::AddItemToSlotPartial(const FPDItemData& ItemData, int32 Quantity, int32 TargetSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (!Items.IsValidIndex(TargetSlotIndex))
	{
		return 0;
	}

	if (!CanAddWeight(ItemData, Quantity))
	{
		BroadcastWeightLimitExceeded();
		return 0;
	}

	const int32 AddedQuantity = FPDItemSlotTransfer::AddItemToSlot(Items[TargetSlotIndex], ItemData, Quantity);
	if (AddedQuantity > 0)
	{
		OnInventoryChanged.Broadcast();

		if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
		{
			QuestComponent->ReportItemAcquired(ItemData.ItemID, AddedQuantity);
			if (ItemData.bIsQuestItem)
			{
				QuestComponent->ReportQuestItemAcquired(ItemData.ItemID, AddedQuantity);
			}
		}
	}

	return AddedQuantity;
}

bool UPDInventoryComponent::MoveSlotQuantityToSlot(int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (!GetOwner()) return false;
	if (!GetOwner()->HasAuthority())
	{
		ServerMoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		return true;
	}
	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (!Items.IsValidIndex(SourceSlotIndex) || !Items.IsValidIndex(TargetSlotIndex) || SourceSlotIndex == TargetSlotIndex || Quantity <= 0)
	{
		return false;
	}

	const bool bMoved = FPDItemSlotTransfer::MoveQuantity(Items[SourceSlotIndex], Items[TargetSlotIndex], Quantity);
	if (bMoved)
	{
		OnInventoryChanged.Broadcast();
	}

	return bMoved;
}

int32 UPDInventoryComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
	if (ItemData.ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	int32 RemainingQuantity = Quantity;
	int32 AddedQuantity = 0;
	bool bEquippedFromThisAdd = false;

	auto HandleAddedItem = [&](int32 InAddedQuantity)
	{
		OnInventoryChanged.Broadcast();

		if (ItemData.WeaponType != EWeaponType::None)
		{
			if (APDPlayerCharacter* PlayerCharacter = FindOwnerPlayerCharacter(this))
			{
				if (!bEquippedFromThisAdd)
				{
					FPDInventorySlot WeaponSlot;
					WeaponSlot.ItemData = ItemData;
					WeaponSlot.Quantity = 1;
					WeaponSlot.bIsEmpty = false;
					WeaponSlot.ModificationLevel = 0;

					bEquippedFromThisAdd = PlayerCharacter->TryAutoEquipWeaponSlot(WeaponSlot);
				}

				if (bEquippedFromThisAdd)
				{
					if (UPDQuickSlotComponent* QuickSlotComponent = PlayerCharacter->GetQuickSlotComponent())
					{
						QuickSlotComponent->AddItemPartial(ItemData, 1);
					}
				}
			}
		}

		if (UPDQuestComponent* QuestComponent = FindOwnerQuestComponent(this))
		{
			QuestComponent->ReportItemAcquired(ItemData.ItemID, InAddedQuantity);
			if (ItemData.bIsQuestItem)
			{
				QuestComponent->ReportQuestItemAcquired(ItemData.ItemID, InAddedQuantity);
			}
		}
	};

	if (ItemData.ItemType == EPDItemType::Equipment || ItemData.WeaponType != EWeaponType::None)
	{
		if (UPDEquipmentComponent* EquipmentComponent = FindOwnerEquipmentComponent(this))
		{
			if (EquipmentComponent->TryEquipNewItem(ItemData))
			{
				RemainingQuantity -= 1;
				AddedQuantity += 1;
				bEquippedFromThisAdd = true;
			}
		}
	}

	if (RemainingQuantity <= 0)
	{
		HandleAddedItem(AddedQuantity);
		return AddedQuantity;
	}

	if (!CanAddWeight(ItemData, RemainingQuantity))
	{
		BroadcastWeightLimitExceeded();
		return AddedQuantity;
	}

	const int32 MaxStack = FMath::Max(1, ItemData.MaxStack);

	if (MaxStack > 1)
	{
		for (FPDInventorySlot& Slot : Items)
		{
			if (!Slot.IsEmpty() &&
				Slot.ItemData.ItemID == ItemData.ItemID &&
				Slot.Quantity < MaxStack)
			{
				const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack - Slot.Quantity);

				Slot.Quantity += AddAmount;
				RemainingQuantity -= AddAmount;
				AddedQuantity += AddAmount;

				if (RemainingQuantity <= 0)
				{
					HandleAddedItem(AddedQuantity);
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
			BroadcastInventoryMessage(FText::FromString(TEXT("인벤토리가 가득 찼습니다.")));
			break;
		}

		const int32 AddAmount = FMath::Min(RemainingQuantity, MaxStack);

		Items[EmptySlot].ItemData = ItemData;
		Items[EmptySlot].Quantity = AddAmount;
		Items[EmptySlot].bIsEmpty = false;

		RemainingQuantity -= AddAmount;
		AddedQuantity += AddAmount;
	}

	if (AddedQuantity > 0)
	{
		HandleAddedItem(AddedQuantity);
	}

	return AddedQuantity;
}


int32 UPDInventoryComponent::AddSlotPartial(const FPDInventorySlot& SourceSlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return 0;
	if (SourceSlot.IsEmpty() || SourceSlot.Quantity <= 0)
	{
		return 0;
	}

	if (!CanAddSlotWeight(SourceSlot, SourceSlot.Quantity))
	{
		BroadcastWeightLimitExceeded();
		return 0;
	}

	if (Items.Num() != GetMaxSlotCount())
	{
		InitializeInventory();
	}

	if (SourceSlot.ItemData.ItemType == EPDItemType::Equipment || SourceSlot.ModificationLevel > 0 || SourceSlot.ItemData.MaxStack <= 1)
	{
		const int32 EmptySlot = FindEmptySlot();
		if (EmptySlot == INDEX_NONE)
		{
			BroadcastInventoryMessage(FText::FromString(TEXT("인벤토리가 가득 찼습니다.")));
			return 0;
		}

		Items[EmptySlot] = SourceSlot;
		Items[EmptySlot].Quantity = FMath::Max(1, SourceSlot.Quantity);
		Items[EmptySlot].bIsEmpty = false;
		OnInventoryChanged.Broadcast();
		return Items[EmptySlot].Quantity;
	}

	return AddItemPartial(SourceSlot.ItemData, SourceSlot.Quantity);
}

void UPDInventoryComponent::InitializeInventory()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 MaxSlotCount = GetMaxSlotCount();

	if (MaxSlotCount <= 0)
	{
		Items.Empty();
		OnInventoryChanged.Broadcast();
		return;
	}

	const int32 OldCount = Items.Num();

	Items.SetNum(MaxSlotCount);

	for (int32 Index = OldCount; Index < Items.Num(); ++Index)
	{
		Items[Index].Clear();
	}

	OnInventoryChanged.Broadcast();
}

void UPDInventoryComponent::ResetInventory()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	Items.SetNum(GetMaxSlotCount());

	for (FPDInventorySlot& Slot : Items)
	{
		Slot.Clear();
	}

	OnInventoryChanged.Broadcast();
}
