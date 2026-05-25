#include "Core/PDPlayerState.h"

#include "Characters/PDPlayerCharacter.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDEquipmentComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Data/PDQuestComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

APDPlayerState::APDPlayerState()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UPDInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentComponent = CreateDefaultSubobject<UPDEquipmentComponent>(TEXT("EquipmentComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UPDQuickSlotComponent>(TEXT("QuickSlotComponent"));
	QuestComponent = CreateDefaultSubobject<UPDQuestComponent>(TEXT("QuestComponent"));

	static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataTableAsset(TEXT("/Game/Main/Data/DT_ItemData.DT_ItemData"));
	if (ItemDataTableAsset.Succeeded())
	{
		ItemDataTable = ItemDataTableAsset.Object;
	}

	ApplyComponentDataDefaults();
}

void APDPlayerState::BeginPlay()
{
	Super::BeginPlay();

	ApplyComponentDataDefaults();
}

void APDPlayerState::ApplyComponentDataDefaults()
{
	if (InventoryComponent && !InventoryComponent->ItemDataTable)
	{
		InventoryComponent->ItemDataTable = ItemDataTable;
	}
}

void APDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APDPlayerState, PersistentData, COND_OwnerOnly);
	DOREPLIFETIME(APDPlayerState, bIsExtracted);
	DOREPLIFETIME(APDPlayerState, bIsRaidDead);
	DOREPLIFETIME(APDPlayerState, RaidStats);
	DOREPLIFETIME(APDPlayerState, bIsTravelReady);
}

void APDPlayerState::SetExtracted(bool bInExtracted)
{
	if (!HasAuthority()) return;
	if (bIsExtracted == bInExtracted) return;
	bIsExtracted = bInExtracted;
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
	ForceNetUpdate();
}

void APDPlayerState::SetRaidDead(bool bInDead)
{
	if (!HasAuthority()) return;
	if (bIsRaidDead == bInDead) return;
	bIsRaidDead = bInDead;
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
	ForceNetUpdate();
}

void APDPlayerState::ResetRaidParticipationState()
{
	if (!HasAuthority()) return;

	// 스탯/스냅샷도 리셋 — StartRaid 마다 새 라이드.
	RaidStats = FPDRaidStats{};
	RaidInitialGold = 0;
	RaidInitialItemQuantity = 0;

	// 결산 ACK 도 리셋.
	if (bIsTravelReady)
	{
		bIsTravelReady = false;
		OnTravelReadyChanged.Broadcast(false);
	}

	if (!bIsExtracted && !bIsRaidDead)
	{
		ForceNetUpdate();
		return;
	}
	bIsExtracted = false;
	bIsRaidDead = false;
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
	ForceNetUpdate();
}

void APDPlayerState::SetRaidStats(const FPDRaidStats& InStats)
{
	if (!HasAuthority()) return;
	RaidStats = InStats;
	ForceNetUpdate();
}

void APDPlayerState::AddKill()
{
	if (!HasAuthority()) return;
	++RaidStats.Kills;
	ForceNetUpdate();
}

void APDPlayerState::SetSurvivalSeconds(float Seconds)
{
	if (!HasAuthority()) return;
	RaidStats.SurvivalSeconds = FMath::Max(0.f, Seconds);
	ForceNetUpdate();
}

void APDPlayerState::SetGoldDelta(int32 Delta)
{
	if (!HasAuthority()) return;
	RaidStats.GoldDelta = Delta;
	ForceNetUpdate();
}

void APDPlayerState::SetItemDelta(int32 Delta)
{
	if (!HasAuthority()) return;
	RaidStats.ItemDelta = Delta;
	ForceNetUpdate();
}

void APDPlayerState::CaptureInitialRaidSnapshot(int32 InGold, int32 InItemQuantity)
{
	if (!HasAuthority()) return;
	RaidInitialGold = FMath::Max(0, InGold);
	RaidInitialItemQuantity = FMath::Max(0, InItemQuantity);
}

void APDPlayerState::OnRep_RaidParticipation()
{
	OnRaidParticipationChanged.Broadcast(bIsExtracted, bIsRaidDead);
}

void APDPlayerState::SetTravelReady(bool bInReady)
{
	if (!HasAuthority()) return;
	if (bIsTravelReady == bInReady) return;
	bIsTravelReady = bInReady;
	OnTravelReadyChanged.Broadcast(bIsTravelReady);
	ForceNetUpdate();
}

void APDPlayerState::OnRep_TravelReady()
{
	OnTravelReadyChanged.Broadcast(bIsTravelReady);
}

APDPlayerCharacter* APDPlayerState::GetPDPlayerCharacter() const
{
	return GetPawn<APDPlayerCharacter>();
}

void APDPlayerState::OnRep_PersistentData()
{
}

void APDPlayerState::InitializePersistentData(const FPDPlayerData& InData)
{
	PersistentData = InData;
	ForceNetUpdate();
}

int32 APDPlayerState::GetTraderReputationExp() const
{
	return FMath::Max(0, PersistentData.TraderReputationExp);
}

int32 APDPlayerState::GetTraderReputationLevel() const
{
	return FMath::Max(1, PersistentData.TraderReputationLevel);
}

void APDPlayerState::SetTraderReputation(int32 InLevel, int32 InExp)
{
	PersistentData.TraderReputationLevel = FMath::Max(1, InLevel);
	PersistentData.TraderReputationExp = FMath::Max(0, InExp);
	ForceNetUpdate();
}

void APDPlayerState::AddTraderReputationExp(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	PersistentData.TraderReputationExp = FMath::Max(0, PersistentData.TraderReputationExp + Amount);
	ForceNetUpdate();
}

void APDPlayerState::ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold)
{
	PersistentData.RaidLoadout = InLoadout;
	PersistentData.RaidGold = FMath::Max(0, InGold);
	PersistentData.Gold = FMath::Max(0, PersistentData.Gold - PersistentData.RaidGold);

	for (const FPDInventorySlot& LoadoutSlot : InLoadout)
	{
		if (LoadoutSlot.IsEmpty())
		{
			continue;
		}

		int32 QuantityToRemove = LoadoutSlot.Quantity;
		for (FPDInventorySlot& StashSlot : PersistentData.StashItems)
		{
			if (StashSlot.IsEmpty() || StashSlot.ItemData.ItemID != LoadoutSlot.ItemData.ItemID)
			{
				continue;
			}

			const int32 RemovedQuantity = FMath::Min(StashSlot.Quantity, QuantityToRemove);
			StashSlot.Quantity -= RemovedQuantity;
			QuantityToRemove -= RemovedQuantity;

			if (StashSlot.Quantity <= 0)
			{
				StashSlot.Clear();
			}

			if (QuantityToRemove <= 0)
			{
				break;
			}
		}
	}

	ForceNetUpdate();
}

void APDPlayerState::ClearRaidLoadout()
{
	PersistentData.RaidLoadout.Empty();
	PersistentData.RaidGold = 0;
	ForceNetUpdate();
}

void APDPlayerState::SetPersistentStashSnapshot(const TArray<FPDInventorySlot>& InStashItems, int32 InUpgradeLevel)
{
	PersistentData.StashItems = InStashItems;
	PersistentData.StashUpgradeLevel = FMath::Max(0, InUpgradeLevel);
	ForceNetUpdate();
}

int32 APDPlayerState::CountPersistentStashItems(FName ItemID, bool bCountAnyItem) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	int32 TotalQuantity = 0;
	for (const FPDInventorySlot& Slot : PersistentData.StashItems)
	{
		if (!Slot.IsEmpty() && (bCountAnyItem || Slot.ItemData.ItemID == ItemID))
		{
			TotalQuantity += Slot.Quantity;
		}
	}
	return TotalQuantity;
}

int32 APDPlayerState::RemovePersistentStashItems(FName ItemID, int32 Quantity, bool bRemoveAnyItem)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	int32 RemainingQuantity = Quantity;
	int32 RemovedQuantity = 0;
	for (int32 Index = PersistentData.StashItems.Num() - 1; Index >= 0 && RemainingQuantity > 0; --Index)
	{
		FPDInventorySlot& Slot = PersistentData.StashItems[Index];
		if (Slot.IsEmpty() || (!bRemoveAnyItem && Slot.ItemData.ItemID != ItemID))
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(RemainingQuantity, Slot.Quantity);
		Slot.Quantity -= RemoveAmount;
		RemainingQuantity -= RemoveAmount;
		RemovedQuantity += RemoveAmount;

		if (Slot.Quantity <= 0)
		{
			Slot.Clear();
		}
	}

	if (RemovedQuantity > 0)
	{
		ForceNetUpdate();
	}

	return RemovedQuantity;
}

void APDPlayerState::TransferInventoryToPersistentStash(const UPDInventoryComponent* SourceInventory)
{
	if (!SourceInventory)
	{
		return;
	}

	for (const FPDInventorySlot& RaidSlot : SourceInventory->Items)
	{
		if (RaidSlot.IsEmpty())
		{
			continue;
		}

		int32 RemainingQuantity = RaidSlot.Quantity;

		if (RaidSlot.ItemData.MaxStack > 1)
		{
			for (FPDInventorySlot& StashSlot : PersistentData.StashItems)
			{
				if (StashSlot.IsEmpty() || StashSlot.ItemData.ItemID != RaidSlot.ItemData.ItemID)
				{
					continue;
				}

				const int32 StackSpace = StashSlot.ItemData.MaxStack - StashSlot.Quantity;
				if (StackSpace <= 0)
				{
					continue;
				}

				const int32 AddedQuantity = FMath::Min(StackSpace, RemainingQuantity);
				StashSlot.Quantity += AddedQuantity;
				StashSlot.bIsEmpty = false;
				RemainingQuantity -= AddedQuantity;

				if (RemainingQuantity <= 0)
				{
					break;
				}
			}
		}

		while (RemainingQuantity > 0)
		{
			FPDInventorySlot NewSlot;
			NewSlot.ItemData = RaidSlot.ItemData;
			NewSlot.ModificationLevel = RaidSlot.ModificationLevel;
			NewSlot.Quantity = FMath::Min(RemainingQuantity, FMath::Max(1, RaidSlot.ItemData.MaxStack));
			NewSlot.bIsEmpty = false;

			PersistentData.StashItems.Add(NewSlot);
			RemainingQuantity -= NewSlot.Quantity;
		}
	}

	PersistentData.Gold += SourceInventory->Gold;
	ForceNetUpdate();
}
