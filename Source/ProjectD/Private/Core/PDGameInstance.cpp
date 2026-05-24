#include "Core/PDGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "Core/PDLocalSessionService.h"
#include "Core/PDSaveGame.h"
#include "Core/PDSessionService.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/PackageName.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/PDLoadingScreenSubsystem.h"

void UPDGameInstance::Init()
{
	Super::Init();
	UAbilitySystemGlobals::Get().InitGlobalData();
	LoadFromDisk();

	const TSubclassOf<UPDSessionService> ResolvedClass =
		SessionServiceClass ? SessionServiceClass : TSubclassOf<UPDSessionService>(UPDLocalSessionService::StaticClass());
	ActiveSessionService = NewObject<UPDSessionService>(this, ResolvedClass);
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

void UPDGameInstance::SetSecureContainerItems(const TArray<FPDInventorySlot>& InSecureContainerItems)
{
	SecureContainerItems = InSecureContainerItems;
}

const TArray<FPDInventorySlot>& UPDGameInstance::GetSecureContainerItems() const
{
	return SecureContainerItems;
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
	// SecureContainer는 세이브파일이 아닌 세션 휴대에만 존재—디스크 로드의는 리셋해서 아이템이 남아있지 않도록 함.
	SecureContainerItems.Reset();
}

void UPDGameInstance::TravelToLevel(TSoftObjectPtr<UWorld> Level, bool bMarkBaseResetPending)
{
	if (Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::TravelToLevel: Level is not set."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bMarkBaseResetPending) bPendingResetToBase=true;

	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::TravelToLevel: clients must request travel through the server."));
		return;
	}

	// 호스트 측 LoadingScreen을 viewport에 즉시 추가.
	// (클라이언트 측은 APDPlayerController::PreClientTravel에서 자동 처리)
	if (UPDLoadingScreenSubsystem* LSS = GetSubsystem<UPDLoadingScreenSubsystem>())
	{
		LSS->ShowImmediate();
	}

	// 한 프레임 지연 후 실제 트래블 — LoadingScreen이 먼저 렌더된 뒤 ServerTravel/OpenLevel cleanup이 시작되도록.
	// 즉시 ServerTravel을 호출하면 같은 프레임에 게임 스레드가 cleanup으로 점유되어 LoadingScreen이 한 프레임도 안 보임.
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this, Level]()
		{
			UWorld* W = GetWorld();
			if (!W) return;

			if (W->GetNetMode() != NM_Standalone)
			{
				const FString LevelPackageName = FPackageName::ObjectPathToPackageName(Level.ToSoftObjectPath().ToString());
				W->ServerTravel(LevelPackageName);
			}
			else
			{
				UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level);
			}
		}));
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
