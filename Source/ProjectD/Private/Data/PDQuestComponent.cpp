#include "Data/PDQuestComponent.h"

#include "Core/PDGameInstance.h"
#include "Items/PDInventoryComponent.h"

namespace
{
	const FName AnyQuestTargetID(TEXT("ANY"));

	bool IsAnyQuestTarget(FName TargetID)
	{
		return TargetID == AnyQuestTargetID;
	}
}

UPDQuestComponent::UPDQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDQuestComponent::AddQuest(const FPDQuestData& QuestData)
{
	if (QuestData.QuestID.IsNone() || FindQuest(QuestData.QuestID))
	{
		return false;
	}

	FPDQuestProgress NewProgress;
	NewProgress.QuestData = QuestData;
	NewProgress.State = EPDQuestState::Active;

	for (const FPDQuestObjective& Objective : QuestData.Objectives)
	{
		const FName ProgressKey = Objective.GetProgressKey();
		if (!ProgressKey.IsNone())
		{
			NewProgress.ObjectiveProgress.Add(ProgressKey, 0);
		}
	}

	for (const FPDQuestObjective& Objective : QuestData.Objectives)
	{
		if (IsItemCountObjective(Objective) && !Objective.TargetID.IsNone())
		{
			const FName ProgressKey = Objective.GetProgressKey();
			if (!ProgressKey.IsNone())
			{
				NewProgress.ObjectiveProgress.FindOrAdd(ProgressKey) = FMath::Clamp(GetInventoryAndStashItemCount(Objective.TargetID), 0, FMath::Max(1, Objective.RequiredCount));
			}
		}
	}

	RefreshQuestState(NewProgress);

	ActiveQuests.Add(NewProgress);
	BroadcastQuestUpdated(QuestData.QuestID, EPDQuestState::Inactive, NewProgress.State);
	return true;
}

bool UPDQuestComponent::AddQuestByID(FName QuestID)
{
	const FPDQuestData* QuestData = FindQuestData(QuestID);
	if (!QuestData)
	{
		return false;
	}

	FPDQuestData RuntimeQuestData = *QuestData;
	RuntimeQuestData.QuestID = QuestID;
	return AddQuest(RuntimeQuestData);
}

bool UPDQuestComponent::UpdateObjectiveProgress(FName QuestID, FName ObjectiveID, int32 Amount)
{
	FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || QuestProgress->State != EPDQuestState::Active || ObjectiveID.IsNone() || Amount <= 0)
	{
		return false;
	}

	for (const FPDQuestObjective& Objective : QuestProgress->QuestData.Objectives)
	{
		const FName ProgressKey = Objective.GetProgressKey();
		if (Objective.ObjectiveID == ObjectiveID || ProgressKey == ObjectiveID)
		{
			return ApplyObjectiveProgress(*QuestProgress, Objective, Amount);
		}
	}

	return false;
}

bool UPDQuestComponent::ReportQuestObjectiveEvent(EPDQuestObjectiveType ObjectiveType, FName TargetID, int32 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}

	bool bUpdatedAny = false;

	for (FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.State != EPDQuestState::Active)
		{
			continue;
		}

		for (const FPDQuestObjective& Objective : QuestProgress.QuestData.Objectives)
		{
			if (DoesObjectiveMatchEvent(Objective, ObjectiveType, TargetID))
			{
				if (IsItemCountObjective(Objective))
				{
					bUpdatedAny |= RefreshItemCountObjective(QuestProgress, Objective);
				}
				else
				{
					bUpdatedAny |= ApplyObjectiveProgress(QuestProgress, Objective, Amount);
				}
			}
		}
	}

	return bUpdatedAny;
}

bool UPDQuestComponent::ReportItemAcquired(FName ItemID, int32 Amount)
{
	return ReportQuestObjectiveEvent(EPDQuestObjectiveType::ItemAcquired, ItemID, Amount);
}

bool UPDQuestComponent::ReportQuestItemAcquired(FName ItemID, int32 Amount)
{
	return ReportQuestObjectiveEvent(EPDQuestObjectiveType::QuestItemAcquired, ItemID, Amount);
}

bool UPDQuestComponent::ReportEnemyKilled(FName EnemyID, int32 Amount)
{
	return ReportQuestObjectiveEvent(EPDQuestObjectiveType::EnemyKilled, EnemyID, Amount);
}

bool UPDQuestComponent::ReportQuestEnemyKilled(FName EnemyID, int32 Amount)
{
	return ReportQuestObjectiveEvent(EPDQuestObjectiveType::QuestEnemyKilled, EnemyID, Amount);
}

bool UPDQuestComponent::ReportLocationReached(FName LocationID, int32 Amount)
{
	return ReportQuestObjectiveEvent(EPDQuestObjectiveType::ReachLocation, LocationID, Amount);
}

bool UPDQuestComponent::ReportNPCTalked(FName NPCID, int32 Amount)
{
	return ReportQuestObjectiveEvent(EPDQuestObjectiveType::TalkToNPC, NPCID, Amount);
}

bool UPDQuestComponent::ReportItemDropped(FName ItemID, int32 Amount)
{
	bool bUpdated = ReportQuestObjectiveEvent(EPDQuestObjectiveType::ItemDropped, ItemID, Amount);

	for (FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.State != EPDQuestState::Active)
		{
			continue;
		}

		for (const FPDQuestObjective& Objective : QuestProgress.QuestData.Objectives)
		{
			if (IsItemCountObjective(Objective) && DoesObjectiveMatchEvent(Objective, Objective.ObjectiveType, ItemID))
			{
				bUpdated |= RefreshItemCountObjective(QuestProgress, Objective);
			}
		}
	}

	return bUpdated;
}

bool UPDQuestComponent::IsQuestCompleted(FName QuestID) const
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	return QuestProgress && (QuestProgress->State == EPDQuestState::Completed || QuestProgress->State == EPDQuestState::Rewarded);
}

bool UPDQuestComponent::GiveReward(FName QuestID, UPDInventoryComponent* InventoryComponent)
{
	FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || QuestProgress->State != EPDQuestState::Completed || !InventoryComponent)
	{
		return false;
	}

	if (!RemoveQuestObjectiveItems(*QuestProgress, InventoryComponent))
	{
		return false;
	}

	for (const FPDItemData& RewardItem : QuestProgress->QuestData.Reward.RewardItems)
	{
		InventoryComponent->AddItemPartial(RewardItem, 1);
	}

	InventoryComponent->AddGold(QuestProgress->QuestData.Reward.RewardGold);

	if (QuestProgress->QuestData.Reward.RewardTraderReputationExp > 0)
	{
		if (UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr)
		{
			GI->SetTraderReputationExp(GI->GetTraderReputationExp() + QuestProgress->QuestData.Reward.RewardTraderReputationExp);
		}
	}

	const EPDQuestState PreviousState = QuestProgress->State;
	QuestProgress->State = EPDQuestState::Rewarded;

	if (TrackedQuestID == QuestID)
	{
		TrackedQuestID = NAME_None;
	}

	BroadcastQuestUpdated(QuestID, PreviousState, QuestProgress->State);
	return true;
}

bool UPDQuestComponent::SetTrackedQuest(FName QuestID)
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || QuestProgress->State == EPDQuestState::Rewarded)
	{
		return false;
	}

	TrackedQuestID = QuestID;
	OnQuestUpdated.Broadcast();
	return true;
}

void UPDQuestComponent::ClearTrackedQuest()
{
	if (!TrackedQuestID.IsNone())
	{
		TrackedQuestID = NAME_None;
		OnQuestUpdated.Broadcast();
	}
}

bool UPDQuestComponent::GetQuestProgress(FName QuestID, FPDQuestProgress& OutQuestProgress) const
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress)
	{
		return false;
	}

	OutQuestProgress = *QuestProgress;
	return true;
}

TArray<FPDQuestProgress> UPDQuestComponent::GetQuestsByState(EPDQuestState State) const
{
	TArray<FPDQuestProgress> Result;

	for (const FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.State == State)
		{
			Result.Add(QuestProgress);
		}
	}

	return Result;
}

TArray<FPDQuestData> UPDQuestComponent::GetAvailableQuests() const
{
	TArray<FPDQuestData> Result;
	if (!QuestTable)
	{
		return Result;
	}

	for (const TPair<FName, uint8*>& RowPair : QuestTable->GetRowMap())
	{
		if (RowPair.Key.IsNone() || !RowPair.Value || FindQuest(RowPair.Key))
		{
			continue;
		}

		const FPDQuestData* Row = reinterpret_cast<const FPDQuestData*>(RowPair.Value);
		if (Row)
		{
			FPDQuestData RuntimeQuestData = *Row;
			RuntimeQuestData.QuestID = RowPair.Key;
			Result.Add(RuntimeQuestData);
		}
	}

	return Result;
}

int32 UPDQuestComponent::GetObjectiveProgress(FName QuestID, FName ObjectiveID) const
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || ObjectiveID.IsNone())
	{
		return 0;
	}

	for (const FPDQuestObjective& Objective : QuestProgress->QuestData.Objectives)
	{
		const FName ProgressKey = Objective.GetProgressKey();
		if (Objective.ObjectiveID == ObjectiveID || ProgressKey == ObjectiveID)
		{
			return QuestProgress->ObjectiveProgress.FindRef(ProgressKey);
		}
	}

	return 0;
}

int32 UPDQuestComponent::GetObjectiveRequiredCount(FName QuestID, FName ObjectiveID) const
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || ObjectiveID.IsNone())
	{
		return 0;
	}

	for (const FPDQuestObjective& Objective : QuestProgress->QuestData.Objectives)
	{
		const FName ProgressKey = Objective.GetProgressKey();
		if (Objective.ObjectiveID == ObjectiveID || ProgressKey == ObjectiveID)
		{
			return FMath::Max(1, Objective.RequiredCount);
		}
	}

	return 0;
}

float UPDQuestComponent::GetQuestProgressRatio(FName QuestID) const
{
	const FPDQuestProgress* QuestProgress = FindQuest(QuestID);
	if (!QuestProgress || QuestProgress->QuestData.Objectives.IsEmpty())
	{
		return 0.0f;
	}

	int32 CurrentTotal = 0;
	int32 RequiredTotal = 0;

	for (const FPDQuestObjective& Objective : QuestProgress->QuestData.Objectives)
	{
		const FName ProgressKey = Objective.GetProgressKey();
		if (ProgressKey.IsNone())
		{
			continue;
		}

		const int32 RequiredCount = FMath::Max(1, Objective.RequiredCount);
		CurrentTotal += FMath::Clamp(QuestProgress->ObjectiveProgress.FindRef(ProgressKey), 0, RequiredCount);
		RequiredTotal += RequiredCount;
	}

	return RequiredTotal > 0 ? static_cast<float>(CurrentTotal) / static_cast<float>(RequiredTotal) : 0.0f;
}

FPDQuestProgress* UPDQuestComponent::FindQuest(FName QuestID)
{
	for (FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.QuestData.QuestID == QuestID)
		{
			return &QuestProgress;
		}
	}

	return nullptr;
}

const FPDQuestProgress* UPDQuestComponent::FindQuest(FName QuestID) const
{
	for (const FPDQuestProgress& QuestProgress : ActiveQuests)
	{
		if (QuestProgress.QuestData.QuestID == QuestID)
		{
			return &QuestProgress;
		}
	}

	return nullptr;
}

const FPDQuestData* UPDQuestComponent::FindQuestData(FName QuestID) const
{
	if (!QuestTable || QuestID.IsNone())
	{
		return nullptr;
	}

	return QuestTable->FindRow<FPDQuestData>(QuestID, TEXT("PDQuestComponent"), false);
}

bool UPDQuestComponent::DoesObjectiveMatchEvent(const FPDQuestObjective& Objective, EPDQuestObjectiveType ObjectiveType, FName TargetID) const
{
	if (Objective.ObjectiveType != ObjectiveType)
	{
		return false;
	}

	return Objective.TargetID.IsNone() || IsAnyQuestTarget(Objective.TargetID) || Objective.TargetID == TargetID;
}

bool UPDQuestComponent::ApplyObjectiveProgress(FPDQuestProgress& QuestProgress, const FPDQuestObjective& Objective, int32 Amount)
{
	if (QuestProgress.State != EPDQuestState::Active || Amount <= 0)
	{
		return false;
	}

	const FName ProgressKey = Objective.GetProgressKey();
	if (ProgressKey.IsNone())
	{
		return false;
	}

	const int32 RequiredCount = FMath::Max(1, Objective.RequiredCount);
	int32& Progress = QuestProgress.ObjectiveProgress.FindOrAdd(ProgressKey);
	const int32 PreviousProgress = Progress;
	const EPDQuestState PreviousState = QuestProgress.State;

	Progress = FMath::Clamp(Progress + Amount, 0, RequiredCount);
	RefreshQuestState(QuestProgress);

	if (PreviousProgress != Progress || PreviousState != QuestProgress.State)
	{
		BroadcastQuestUpdated(QuestProgress.QuestData.QuestID, PreviousState, QuestProgress.State);
		return true;
	}

	return false;
}


bool UPDQuestComponent::IsItemCountObjective(const FPDQuestObjective& Objective) const
{
	return Objective.ObjectiveType == EPDQuestObjectiveType::ItemAcquired || Objective.ObjectiveType == EPDQuestObjectiveType::QuestItemAcquired;
}

int32 UPDQuestComponent::GetInventoryAndStashItemCount(FName ItemID, const UPDInventoryComponent* InventoryComponent) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	const bool bCountAnyItem = IsAnyQuestTarget(ItemID);
	int32 TotalQuantity = 0;
	const UPDInventoryComponent* SourceInventory = InventoryComponent;
	if (!SourceInventory)
	{
		SourceInventory = GetOwner() ? GetOwner()->FindComponentByClass<UPDInventoryComponent>() : nullptr;
	}

	if (SourceInventory)
	{
		for (const FPDInventorySlot& Slot : SourceInventory->Items)
		{
			if (!Slot.IsEmpty() && (bCountAnyItem || Slot.ItemData.ItemID == ItemID))
			{
				TotalQuantity += Slot.Quantity;
			}
		}
	}

	const UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr;
	if (GI)
	{
		for (const FPDInventorySlot& Slot : GI->GetStashItems())
		{
			if (!Slot.IsEmpty() && (bCountAnyItem || Slot.ItemData.ItemID == ItemID))
			{
				TotalQuantity += Slot.Quantity;
			}
		}
	}

	return TotalQuantity;
}

bool UPDQuestComponent::RefreshItemCountObjective(FPDQuestProgress& QuestProgress, const FPDQuestObjective& Objective)
{
	if (QuestProgress.State != EPDQuestState::Active || Objective.TargetID.IsNone())
	{
		return false;
	}

	const FName ProgressKey = Objective.GetProgressKey();
	if (ProgressKey.IsNone())
	{
		return false;
	}

	const int32 RequiredCount = FMath::Max(1, Objective.RequiredCount);
	int32& Progress = QuestProgress.ObjectiveProgress.FindOrAdd(ProgressKey);
	const int32 PreviousProgress = Progress;
	const EPDQuestState PreviousState = QuestProgress.State;

	Progress = FMath::Clamp(GetInventoryAndStashItemCount(Objective.TargetID), 0, RequiredCount);
	RefreshQuestState(QuestProgress);

	if (PreviousProgress != Progress || PreviousState != QuestProgress.State)
	{
		BroadcastQuestUpdated(QuestProgress.QuestData.QuestID, PreviousState, QuestProgress.State);
		return true;
	}

	return false;
}

void UPDQuestComponent::RefreshAllItemCountObjectives(FPDQuestProgress& QuestProgress)
{
	if (QuestProgress.State != EPDQuestState::Active)
	{
		return;
	}

	for (const FPDQuestObjective& Objective : QuestProgress.QuestData.Objectives)
	{
		if (IsItemCountObjective(Objective))
		{
			RefreshItemCountObjective(QuestProgress, Objective);
		}
	}
}

int32 UPDQuestComponent::RemoveQuestItemsFromStash(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	UPDGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPDGameInstance>() : nullptr;
	if (!GI)
	{
		return 0;
	}

	TArray<FPDInventorySlot> StashItems = GI->GetStashItems();
	const bool bRemoveAnyItem = IsAnyQuestTarget(ItemID);
	int32 RemainingQuantity = Quantity;
	int32 RemovedQuantity = 0;

	for (int32 Index = StashItems.Num() - 1; Index >= 0 && RemainingQuantity > 0; --Index)
	{
		FPDInventorySlot& Slot = StashItems[Index];
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
		GI->SetStashItems(StashItems);
	}

	return RemovedQuantity;
}

bool UPDQuestComponent::RemoveQuestObjectiveItems(FPDQuestProgress& QuestProgress, UPDInventoryComponent* InventoryComponent)
{
	if (!InventoryComponent)
	{
		return false;
	}

	TMap<FName, int32> RequiredItems;
	for (const FPDQuestObjective& Objective : QuestProgress.QuestData.Objectives)
	{
		if (Objective.ObjectiveType == EPDQuestObjectiveType::QuestItemAcquired && !Objective.TargetID.IsNone())
		{
			RequiredItems.FindOrAdd(Objective.TargetID) += FMath::Max(1, Objective.RequiredCount);
		}
	}

	for (const TPair<FName, int32>& Pair : RequiredItems)
	{
		if (GetInventoryAndStashItemCount(Pair.Key, InventoryComponent) < Pair.Value)
		{
			return false;
		}
	}

	for (const TPair<FName, int32>& Pair : RequiredItems)
	{
		int32 RemainingQuantity = Pair.Value;
		int32 InventoryQuantity = 0;

		const bool bRemoveAnyItem = IsAnyQuestTarget(Pair.Key);

		for (const FPDInventorySlot& Slot : InventoryComponent->Items)
		{
			if (!Slot.IsEmpty() && (bRemoveAnyItem || Slot.ItemData.ItemID == Pair.Key))
			{
				InventoryQuantity += Slot.Quantity;
			}
		}

		int32 RemoveFromInventory = FMath::Min(RemainingQuantity, InventoryQuantity);
		if (RemoveFromInventory > 0)
		{
			if (bRemoveAnyItem)
			{
				for (int32 Index = InventoryComponent->Items.Num() - 1; Index >= 0 && RemoveFromInventory > 0; --Index)
				{
					const FPDInventorySlot& Slot = InventoryComponent->Items[Index];
					if (Slot.IsEmpty())
					{
						continue;
					}

					const int32 RemoveAmount = FMath::Min(RemoveFromInventory, Slot.Quantity);
					if (InventoryComponent->RemoveItemFromSlot(Index, RemoveAmount))
					{
						RemoveFromInventory -= RemoveAmount;
						RemainingQuantity -= RemoveAmount;
					}
				}
			}
			else if (InventoryComponent->RemoveItem(Pair.Key, RemoveFromInventory))
			{
				RemainingQuantity -= RemoveFromInventory;
			}
		}

		if (RemainingQuantity > 0)
		{
			const int32 RemovedFromStash = RemoveQuestItemsFromStash(Pair.Key, RemainingQuantity);
			RemainingQuantity -= RemovedFromStash;
		}

		if (RemainingQuantity > 0)
		{
			return false;
		}
	}

	return true;
}

void UPDQuestComponent::RefreshQuestState(FPDQuestProgress& QuestProgress)
{
	if (QuestProgress.State != EPDQuestState::Active || QuestProgress.QuestData.Objectives.IsEmpty())
	{
		return;
	}

	for (const FPDQuestObjective& Objective : QuestProgress.QuestData.Objectives)
	{
		const FName ProgressKey = Objective.GetProgressKey();
		if (ProgressKey.IsNone())
		{
			return;
		}

		const int32 CurrentProgress = QuestProgress.ObjectiveProgress.FindRef(ProgressKey);
		if (CurrentProgress < FMath::Max(1, Objective.RequiredCount))
		{
			return;
		}
	}

	QuestProgress.State = EPDQuestState::Completed;
}

void UPDQuestComponent::BroadcastQuestUpdated(FName QuestID, EPDQuestState PreviousState, EPDQuestState NewState)
{
	if (PreviousState != NewState)
	{
		OnQuestStateChanged.Broadcast(QuestID, NewState);
	}

	OnQuestUpdated.Broadcast();
}
