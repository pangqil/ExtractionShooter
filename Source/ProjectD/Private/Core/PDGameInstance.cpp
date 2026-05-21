#include "Core/PDGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "Core/PDSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/PackageName.h"
#include "Kismet/GameplayStatics.h"

void UPDGameInstance::Init()
{
	Super::Init();
	UAbilitySystemGlobals::Get().InitGlobalData();
	LoadFromDisk();
}

void UPDGameInstance::SetPlayerData(const FPDPlayerData& InData)
{
	PlayerData=InData;
}

FString UPDGameInstance::GetSaveKeyForController(const APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return TEXT("LocalPlayer");
	}

	if (const APlayerState* PlayerState = PlayerController->PlayerState)
	{
		const FString PlayerName = PlayerState->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			return FString::Printf(TEXT("%s_%d"), *PlayerName, PlayerState->GetPlayerId());
		}
	}

	return PlayerController->GetName();
}

FPDPlayerData UPDGameInstance::LoadPlayerDataFromDisk(const FString& SaveKey, bool bUseLegacyFallback) const
{
	const FString SlotName = MakePlayerSlotName(SaveKey);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UPDSaveGame::UserIndex))
	{
		return bUseLegacyFallback ? PlayerData : FPDPlayerData();
	}

	const UPDSaveGame* SaveObject = Cast<UPDSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UPDSaveGame::UserIndex));
	return SaveObject ? SaveObject->PlayerData : (bUseLegacyFallback ? PlayerData : FPDPlayerData());
}

void UPDGameInstance::SavePlayerDataToDisk(const FString& SaveKey, const FPDPlayerData& InData) const
{
	UPDSaveGame* SaveObject = Cast<UPDSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDSaveGame::StaticClass()));
	if (!SaveObject)
	{
		return;
	}

	SaveObject->PlayerData = InData;
	UGameplayStatics::SaveGameToSlot(SaveObject, MakePlayerSlotName(SaveKey), UPDSaveGame::UserIndex);
}

void UPDGameInstance::SetStashItems(const TArray<FPDInventorySlot>& InStashItems)
{
	PlayerData.StashItems=InStashItems;
}

const TArray<FPDInventorySlot>& UPDGameInstance::GetStashItems() const
{
	return PlayerData.StashItems;
}

void UPDGameInstance::SetStashUpgradeLevel(int32 InUpgradeLevel)
{
	PlayerData.StashUpgradeLevel = FMath::Max(0, InUpgradeLevel);
}

int32 UPDGameInstance::GetStashUpgradeLevel() const
{
	return FMath::Max(0, PlayerData.StashUpgradeLevel);
}

void UPDGameInstance::SetTraderReputation(int32 InLevel, int32 InExp)
{
	PlayerData.TraderReputationLevel = FMath::Max(1, InLevel);
	PlayerData.TraderReputationExp = FMath::Max(0, InExp);
}

void UPDGameInstance::SetTraderReputationExp(int32 InExp)
{
	PlayerData.TraderReputationExp = FMath::Max(0, InExp);
}

void UPDGameInstance::SetTraderReputationLevel(int32 InLevel)
{
	PlayerData.TraderReputationLevel = FMath::Max(1, InLevel);
}

int32 UPDGameInstance::GetTraderReputationExp() const
{
	return FMath::Max(0, PlayerData.TraderReputationExp);
}

int32 UPDGameInstance::GetTraderReputationLevel() const
{
	return FMath::Max(1, PlayerData.TraderReputationLevel);
}

void UPDGameInstance::ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold)
{
	PlayerData.RaidLoadout = InLoadout;
	PlayerData.RaidGold    = FMath::Max(0, InGold);

	PlayerData.Gold = FMath::Max(0, PlayerData.Gold - PlayerData.RaidGold);

	for (const FPDInventorySlot& LoadoutSlot : InLoadout)
	{
		if (LoadoutSlot.IsEmpty()) continue;
		int32 ToRemove = LoadoutSlot.Quantity;
		for (FPDInventorySlot& StashSlot : PlayerData.StashItems)
		{
			if (StashSlot.IsEmpty()) continue;
			if (StashSlot.ItemData.ItemID != LoadoutSlot.ItemData.ItemID) continue;
			int32 Removed = FMath::Min(StashSlot.Quantity, ToRemove);
			StashSlot.Quantity -= Removed;
			if (StashSlot.Quantity <= 0) StashSlot.Clear();
			ToRemove -= Removed;
			if (ToRemove <= 0) break;
		}
	}

	SaveToDisk();
}

void UPDGameInstance::ClearRaidLoadout()
{
	PlayerData.RaidLoadout.Empty();
	PlayerData.RaidGold = 0;
}

void UPDGameInstance::SaveToDisk()
{
	UPDSaveGame* SaveObject=Cast<UPDSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDSaveGame::StaticClass()));
	if (!SaveObject) return;
	SaveObject->PlayerData=PlayerData;
	UGameplayStatics::SaveGameToSlot(SaveObject, UPDSaveGame::SlotName, UPDSaveGame::UserIndex);
}

void UPDGameInstance::LoadFromDisk()
{
	if (!UGameplayStatics::DoesSaveGameExist(UPDSaveGame::SlotName, UPDSaveGame::UserIndex)) return;
	UPDSaveGame* SaveObject=Cast<UPDSaveGame>(UGameplayStatics::LoadGameFromSlot(UPDSaveGame::SlotName, UPDSaveGame::UserIndex));
	if (!SaveObject) return;
	PlayerData=SaveObject->PlayerData;
}

void UPDGameInstance::TravelToBaseLevel(bool bMarkResetPending)
{
	if (BaseLevel.IsNull())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bMarkResetPending) bPendingResetToBase=true;

	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::TravelToBaseLevel: clients must request travel through the server."));
		return;
	}

	if (World->GetNetMode() != NM_Standalone)
	{
		const FString BaseLevelPackageName = FPackageName::ObjectPathToPackageName(BaseLevel.ToSoftObjectPath().ToString());
		World->ServerTravel(BaseLevelPackageName);
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, BaseLevel);
}

bool UPDGameInstance::ConsumePendingResetToBase()
{
	const bool bWasPending=bPendingResetToBase;
	bPendingResetToBase=false;
	return bWasPending;
}

FString UPDGameInstance::MakePlayerSlotName(const FString& SaveKey) const
{
	return FString::Printf(TEXT("%s_%s"), *UPDSaveGame::SlotName, *SanitizeSaveKey(SaveKey));
}

FString UPDGameInstance::SanitizeSaveKey(const FString& SaveKey) const
{
	FString Result = SaveKey.IsEmpty() ? TEXT("LocalPlayer") : SaveKey;
	const TCHAR InvalidChars[] = TEXT("\\/:*?\"<>|. ");
	for (int32 Index = 0; InvalidChars[Index] != TCHAR('\0'); ++Index)
	{
		Result.ReplaceCharInline(InvalidChars[Index], TCHAR('_'));
	}
	return Result;
}
