#include "Items/Containers/PDLootComponent.h"

#include "Engine/DataTable.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Net/UnrealNetwork.h"

UPDLootComponent::UPDLootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDLootComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureSlotCount();
}

void UPDLootComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_None: Î™®Îì† ?µÏ?Î≤ÑÏóêÍ≤??ôÍ∏∞????LootBox ???ÑÍµ¨???ôÏùº??ÏΩòÌÖêÏ∏†Î? Î¥êÏïº ??
	DOREPLIFETIME_CONDITION(UPDLootComponent, LootItems, COND_None);
}

void UPDLootComponent::OnRep_LootItems()
{
	OnLootChanged.Broadcast();
}

void UPDLootComponent::EnsureSlotCount()
{
	const int32 Required = GetMaxSlotCount();
	if (LootItems.Num() < Required)
	{
		LootItems.SetNum(Required);
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FPDItemContainerOps::EnsureInstanceIDs(LootItems);
	}
}

int32 UPDLootComponent::FindEmptySlot() const
{
	return FPDItemContainerOps::FindEmptySlot(LootItems);
}

bool UPDLootComponent::AddItemByID(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0) return false;

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] PDLootComponent: ItemDataTable ÎØ∏ÏÑ§?? BP_PDLootBoxActor ??LootComponent ?îÌÖå?ºÏóê??DT_ItemData ÏßÄ???ÑÏöî."),
			*GetNameSafe(GetOwner()));
		return false;
	}

	// RowName ???ÑÎãå row ?¥Î? ItemID ?ÑÎìúÎ°?Í≤Ä????Stash ?Ä ?ôÏùº ?ïÏ±Ö.
	// DT ?îÏûê?¥ÎÑà ?¥ÏòÅ??RowName ?Ä ?ÑÏùòÍ∞íÏù¥Í≥?ItemID ?ÑÎìúÍ∞Ä canonical key ??
	TArray<FPDItemData*> Rows;
	ItemDataTable->GetAllRows<FPDItemData>(TEXT("PDLootComponent::AddItemByID"), Rows);

	for (const FPDItemData* Row : Rows)
	{
		if (Row && Row->ItemID == ItemID)
		{
			return AddItemPartial(*Row, Quantity) > 0;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[%s] PDLootComponent: ItemDataTable ??ItemID ?ÑÎìú '%s' ?ºÏπò?òÎäî row ?ÜÏùå. DT_ItemData ??ItemID Ïª¨Îüº ?ïÏù∏."),
		*GetNameSafe(GetOwner()), *ItemID.ToString());
	return false;
}

int32 UPDLootComponent::AddItemPartial(const FPDItemData& ItemData, int32 Quantity)
{
	if (Quantity <= 0) return 0;
	EnsureSlotCount();

	const int32 Added = FPDItemContainerOps::AddItem(LootItems, ItemData, Quantity);
	if (Added > 0)
	{
		OnLootChanged.Broadcast();
	}
	return Added;
}

bool UPDLootComponent::TakeSlotToInventory(int32 LootSlotIndex, UPDInventoryComponent* TargetInventory, int32 Quantity)
{
	if (!LootItems.IsValidIndex(LootSlotIndex) || !TargetInventory) return false;

	FPDInventorySlot& Slot = LootItems[LootSlotIndex];
	if (Slot.IsEmpty()) return false;

	const int32 RequestQty = (Quantity < 0) ? Slot.Quantity : FMath::Min(Quantity, Slot.Quantity);
	if (RequestQty <= 0) return false;

	FPDInventorySlot TransferSlot = Slot;
	TransferSlot.Quantity = RequestQty;

	const int32 Added = TargetInventory->AddSlotPartial(TransferSlot);
	if (Added <= 0) return false;

	Slot.Quantity -= Added;
	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	OnLootChanged.Broadcast();
	return true;
}
