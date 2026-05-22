#include "Core/PDGameMode.h"
#include "Core/PDGameState.h"
#include "Core/PDGameInstance.h"
#include "Items/PDSecureContainerComponent.h"
#include "Core/PDPlayerState.h"
#include "Items/PDInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "Type/Types.h"

APDGameMode::APDGameMode()
{
	PlayerStateClass = APDPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void APDGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	InitializePlayerStateFromSave(NewPlayer);
}

void APDGameMode::StartRaid()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		InitializePlayerInventoryFromLoadout(PC);
	}

	SetRaidState(ERaidState::InProgress);
}

void APDGameMode::TravelToRaidLevel(FName RaidMapName)
{
	if (RaidMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("APDGameMode::TravelToRaidLevel: RaidMapName is not set."));
		return;
	}

	StartRaid();
	GetWorld()->ServerTravel(RaidMapName.ToString());
}

void APDGameMode::TravelToBaseLevel(bool bMarkResetPending)
{
	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		GI->TravelToLevel(GI->GetBaseLevel(), bMarkResetPending);
	}
}

void APDGameMode::RequestExtraction(APlayerController* PC)
{
	if (!PC||CurrentRaidState!=ERaidState::InProgress) return;
	SetRaidState(ERaidState::Extracting);
	EndRaid(true);
	ScheduleReturnToBaseTravel(ExtractionToTravelDelay);
}

void APDGameMode::EndRaid(bool bSuccess)
{
	if (CurrentRaidState == ERaidState::Ended) return;
	SetRaidState(ERaidState::Ended);

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();

	if (bSuccess)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			TransferPlayerInventoryToStash(PC);
			SavePlayerStateToDisk(PC);

			// SecureContainer는 생존 시 내용물을 유지(GI 메모리에 저장해 다음 레이드로 이월)
			if (PC)
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					if (UPDSecureContainerComponent* SecureContainer = Pawn->FindComponentByClass<UPDSecureContainerComponent>())
					{
						SecureContainer->SaveSecureContainer();
					}
				}
			}
		}
	}
	else if (GI)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			SavePlayerStateToDisk(It->Get());
		}
		// 사망 시 SecureContainer도 날린다(탈해 실패 규칙).
		GI->SetSecureContainerItems({});
	}


	OnRaidEnded(bSuccess);
}

void APDGameMode::OnPlayerDied(APlayerController* PC, AActor* Killer)
{
	if (!PC) return;
	if (CurrentRaidState == ERaidState::Ended) return;
	EndRaid(false);
	ScheduleReturnToBaseTravel(DeathToTravelDelay);
}

void APDGameMode::HandleReturnToBaseTravel()
{
	TravelToBaseLevel(true);
}

void APDGameMode::ScheduleReturnToBaseTravel(float Delay)
{
	GetWorldTimerManager().ClearTimer(ReturnToBaseTravelTimerHandle);
	GetWorldTimerManager().SetTimer(
		ReturnToBaseTravelTimerHandle,
		this,
		&APDGameMode::HandleReturnToBaseTravel,
		FMath::Max(0.f, Delay),
		false);
}

void APDGameMode::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;
	CurrentRaidState=NewState;
	if (APDGameState* GS=GetGameState<APDGameState>()) GS->SetRaidState(NewState);
	OnRaidStateChanged(NewState);
}


void APDGameMode::InitializePlayerStateFromSave(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!PDPlayerState || !GI)
	{
		return;
	}

	PDPlayerState->InitializePersistentData(GI->LoadPlayerDataFromDisk(GI->GetSaveKeyForController(PC), PC->IsLocalController()));
}

void APDGameMode::SavePlayerStateToDisk(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!PDPlayerState || !GI)
	{
		return;
	}

	GI->SavePlayerDataToDisk(GI->GetSaveKeyForController(PC), PDPlayerState->GetPersistentData());
}

void APDGameMode::InitializePlayerInventoryFromLoadout(APlayerController* PC)
{
	if (!PC) return;

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDInventoryComponent* Inventory = PDPlayerState ? PDPlayerState->GetInventoryComponent() : nullptr;
	if (!Inventory) return;

	Inventory->ResetInventory();

	for (const FPDInventorySlot& Slot : PDPlayerState->GetRaidLoadout())
	{
		if (!Slot.IsEmpty())
		{
			Inventory->AddItem(Slot.ItemData, Slot.Quantity);
		}
	}

	Inventory->Gold = PDPlayerState->GetRaidGold();
	PDPlayerState->ClearRaidLoadout();
}

void APDGameMode::TransferPlayerInventoryToStash(APlayerController* PC)
{
	if (!PC) return;

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDInventoryComponent* Inventory = PDPlayerState ? PDPlayerState->GetInventoryComponent() : nullptr;
	if (!Inventory) return;

	PDPlayerState->TransferInventoryToPersistentStash(Inventory);
}
