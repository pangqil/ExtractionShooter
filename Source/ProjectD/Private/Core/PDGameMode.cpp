#include "Core/PDGameMode.h"
#include "Core/PDGameState.h"
#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"
#include "Items/Containers/PDSecureContainerComponent.h"
#include "Core/PDPlayerState.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Game/PDRaidStats.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Type/Types.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDRaid, Log, All);

APDGameMode::APDGameMode()
{
	PlayerStateClass = APDPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void APDGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	InitializePlayerStateFromSave(NewPlayer);

	UE_LOG(LogPDRaid, Log, TEXT("PostLogin: PC=%s NumPlayers=%d"),
		*GetNameSafe(NewPlayer),
		GetWorld() ? GetWorld()->GetNumPlayerControllers() : -1);
}

void APDGameMode::StartRaid()
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	RaidStartServerTime = GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);

	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		InitializePlayerInventoryFromLoadout(PC);

		if (APDPlayerState* PS = PC ? PC->GetPlayerState<APDPlayerState>() : nullptr)
		{
			PS->ResetRaidParticipationState();
		}

		// Step 2-C: Ï¥àÍ∏∞ Í≥®Îìú/?ÑÏù¥???òÎüâ ?§ÎÉÖ??(?¨Îßù/Ï∂îÏ∂ú ?úÏ†ê??Delta Í≥ÑÏÇ∞??.
		CaptureInitialRaidSnapshotFor(PC);
		++PlayerCount;
	}

	SetRaidState(ERaidState::InProgress);
	UE_LOG(LogPDRaid, Log, TEXT("StartRaid: Participants=%d StartTime=%.2f"), PlayerCount, RaidStartServerTime);
}

void APDGameMode::TravelToRaidLevel(FName RaidMapName)
{
	if (RaidMapName.IsNone())
	{
		UE_LOG(LogPDRaid, Warning, TEXT("TravelToRaidLevel: RaidMapName is not set."));
		return;
	}

	StartRaid();
	UE_LOG(LogPDRaid, Log, TEXT("TravelToRaidLevel: ServerTravel to %s"), *RaidMapName.ToString());
	GetWorld()->ServerTravel(RaidMapName.ToString());
}

void APDGameMode::TravelToBaseLevel(bool bMarkResetPending)
{
	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		UE_LOG(LogPDRaid, Log, TEXT("TravelToBaseLevel: bMarkResetPending=%d"), bMarkResetPending ? 1 : 0);
		GI->TravelToLevel(GI->GetBaseLevel(), bMarkResetPending);
	}
}

void APDGameMode::RequestExtraction(APlayerController* PC)
{
	if (!PC)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: null PC, ignored"));
		return;
	}

	if (CurrentRaidState != ERaidState::InProgress)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: ignored, state=%d PC=%s"),
			static_cast<int32>(CurrentRaidState), *PC->GetName());
		return;
	}

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: PC=%s has no PDPlayerState"), *PC->GetName());
		return;
	}

	if (PS->IsExtracted() || PS->IsRaidDead())
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: PC=%s already finalized (extracted=%d dead=%d)"),
			*PC->GetName(), PS->IsExtracted() ? 1 : 0, PS->IsRaidDead() ? 1 : 0);
		return;
	}

	UE_LOG(LogPDRaid, Log, TEXT("RequestExtraction: PC=%s -> mark extracted"), *PC->GetName());
	PS->SetExtracted(true);

	// Step 2-C: ?∏Î≤§?†Î¶¨Í∞Ä stash Î°???≤®ÏßÄÍ∏??ÑÏóê Delta ?ïÏ†ï.
	FinalizeRaidStatsOnExtraction(PC);

	ProcessExtractionForPlayer(PC);

	// Step 2-B: Ï∂îÏ∂ú?êÎ? Í¥Ä?ÑÌïò???§Î•∏ PC Í∞Ä ?àÏúºÎ©??§Ïùå ?ùÏ°¥?êÎ°ú ?úÌôò.
	NotifyOthersWatchingFinalized(PC);

	EvaluateRaidEnd();
}

void APDGameMode::OnPlayerDied(APlayerController* PC, AActor* Killer)
{
	if (!PC)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("OnPlayerDied: null PC, ignored"));
		return;
	}
	if (CurrentRaidState == ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("OnPlayerDied: PC=%s ignored, raid already Ended"), *PC->GetName());
		return;
	}

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (PS)
	{
		if (PS->IsRaidDead())
		{
			UE_LOG(LogPDRaid, Verbose, TEXT("OnPlayerDied: PC=%s already marked dead"), *PC->GetName());
			return;
		}
		PS->SetRaidDead(true);
	}

	UE_LOG(LogPDRaid, Log, TEXT("OnPlayerDied: PC=%s Killer=%s Alive=%d Dead=%d Extracted=%d"),
		*PC->GetName(),
		*GetNameSafe(Killer),
		CountAlivePlayers(), CountDeadPlayers(), CountExtractedPlayers());

	ProcessDeathForPlayer(PC);

	// Step 2-C: ?¨Îßù??Í≤∞ÏÇ∞ ?§ÌÉØ ?ïÏ†ï + ?¨Îü¨?êÍ≤å Kill ?¨Î†à??
	FinalizeRaidStatsOnDeath(PC);
	CreditKillToShooter(Killer, PC);

	// Step 2-B: ?¨Îßù?êÎäî Ï≤??ùÏ°¥???úÏ†ê?ºÎ°ú Í¥Ä???ÑÌôò,
	// Ï£ΩÏ? PCÎ•?Î≥¥Í≥† ?àÎçò ?§Î•∏ Í¥Ä?ÑÏûê?§Ï? ?§Ïùå ?ùÏ°¥?êÎ°ú ?êÎèô ?úÌôò.
	HandleSpectatorTransitionForDeath(PC);
	NotifyOthersWatchingFinalized(PC);

	EvaluateRaidEnd();
}

void APDGameMode::EvaluateRaidEnd()
{
	if (CurrentRaidState != ERaidState::InProgress)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("EvaluateRaidEnd: skipped, state=%d"),
			static_cast<int32>(CurrentRaidState));
		return;
	}

	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS || GS->PlayerArray.Num() == 0)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("EvaluateRaidEnd: empty PlayerArray, holding off"));
		return;
	}

	int32 Alive = 0;
	int32 Extracted = 0;
	int32 Dead = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (!PS) continue;

		if (PS->IsExtracted())     ++Extracted;
		else if (PS->IsRaidDead()) ++Dead;
		else                       ++Alive;
	}

	UE_LOG(LogPDRaid, Log, TEXT("EvaluateRaidEnd: Alive=%d Dead=%d Extracted=%d (Total=%d)"),
		Alive, Dead, Extracted, GS->PlayerArray.Num());

	if (Alive > 0)
	{
		UE_LOG(LogPDRaid, Log, TEXT("EvaluateRaidEnd: %d player(s) still alive, raid continues"), Alive);
		return;
	}

	const bool bAnyExtracted = Extracted > 0;
	UE_LOG(LogPDRaid, Log, TEXT("EvaluateRaidEnd: all participants finalized -> EndRaid(bSuccess=%d)"),
		bAnyExtracted ? 1 : 0);
	EndRaid(bAnyExtracted);
}

void APDGameMode::EndRaid(bool bSuccess)
{
	if (CurrentRaidState == ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("EndRaid: already Ended, ignored"));
		return;
	}
	SetRaidState(ERaidState::Ended);

	UE_LOG(LogPDRaid, Log, TEXT("EndRaid: bSuccess=%d Alive=%d Dead=%d Extracted=%d"),
		bSuccess ? 1 : 0,
		CountAlivePlayers(), CountDeadPlayers(), CountExtractedPlayers());

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();

	// ?ïÏÇ∞ ÎßàÍ∞ê: Ï∂îÏ∂ú?êÎäî RequestExtraction ?®Í≥Ñ?êÏÑú ?¥Î? Ï≤òÎ¶¨?? ?¨Í∏∞?úÎäî ?¨Îßù?êÎßå ÎßàÏ? Ï≤òÎ¶¨.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS) continue;

		if (PS->IsExtracted())
		{
			UE_LOG(LogPDRaid, Log, TEXT("EndRaid: PC=%s already extracted (kept items/gold)"), *PC->GetName());
			continue;
		}

		// ?¨Îßù?? ?∏Î≤§?†Î¶¨/RaidGold ?êÏã§. ?ÅÍµ¨ ?∞Ïù¥???§ÌÉú???âÌåê ??Îß??Ä??
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: PC=%s dead -> drop raid inventory, save persistent only"),
			*PC->GetName());
		SavePlayerStateToDisk(PC);
	}

	// SecureContainer ?ÑÏó≠ Í∑úÏπô: ?¥Îäê ?ÑÍµ¨??Ï∂îÏ∂ú Î™??àÏúºÎ©?Î©îÎ™®Î¶?Ï∫êÏãú ÎπÑÏ?(?§Ïùå ?àÏù¥??ÎπàÏÜê).
	if (!bSuccess && GI)
	{
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: no survivors, clearing GI SecureContainer cache"));
		GI->SetSecureContainerItems({});
	}

	// BP ?∞Ï∂ú ?????¨Ïö¥???¥Ìéô?????úÎ≤Ñ ?¨Ïù¥???∞Ï∂ú?? Í≤∞ÏÇ∞ ?ÑÏ†Ø ?∏Ïãú???ÑÎûò ClientRPC Î°??ºÏõê??
	OnRaidEnded(bSuccess);

	// ?µÏÖò B: Î™®Îì† PC ??Í≤∞ÏÇ∞ ?ÑÏ†Ø push ClientRPC.
	TArray<FPDPlayerRaidEntryData> Entries;
	BuildRaidEndEntries(Entries);
	const float Duration = GetCurrentRaidElapsedSeconds();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APDPlayerController* PDPC = Cast<APDPlayerController>(It->Get());
		if (!PDPC) continue;
		PDPC->Client_ShowRaidEndTransition(bSuccess, Entries, Duration);
	}

	// Î™®Îì† PCÍ∞Ä ACK?òÍ±∞??TravelGateTimeoutSeconds Í≤ΩÍ≥º ??ServerTravel. ??Ï§?Îπ†Î•∏ Ï™ΩÏù¥ Î∞úÎèô.
	UE_LOG(LogPDRaid, Log, TEXT("EndRaid: travel gate armed (timeout=%.1fs) Entries=%d Duration=%.1fs"),
		TravelGateTimeoutSeconds, Entries.Num(), Duration);
	GetWorldTimerManager().SetTimer(
		TravelTimeoutTimerHandle,
		this,
		&APDGameMode::OnTravelTimeoutExpired,
		TravelGateTimeoutSeconds,
		false);
}

void APDGameMode::ProcessExtractionForPlayer(APlayerController* PC)
{
	if (!PC) return;

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	UE_LOG(LogPDRaid, Log, TEXT("ProcessExtraction: PC=%s -> transfer inventory to stash"),
		*PC->GetName());

	TransferPlayerInventoryToStash(PC);
	SavePlayerStateToDisk(PC);

	// SecureContainer ???ùÏ°¥(Ï∂îÏ∂ú) ???§Ïùå ?àÏù¥?úÎ°ú ?¥Ïõî.
	if (APawn* Pawn = PC->GetPawn())
	{
		if (UPDSecureContainerComponent* SecureContainer = Pawn->FindComponentByClass<UPDSecureContainerComponent>())
		{
			SecureContainer->SaveSecureContainer();
			UE_LOG(LogPDRaid, Verbose, TEXT("ProcessExtraction: PC=%s saved SecureContainer"), *PC->GetName());
		}
	}
}

void APDGameMode::ProcessDeathForPlayer(APlayerController* PC)
{
	if (!PC) return;

	// ?¨Îßù?êÎäî ?¥Ï≤¥?òÏ? ?äÏùå(?∏Î≤§/RaidGold ?êÏã§). ?§Î•∏ ?ÅÍµ¨ ?∞Ïù¥?∞Îäî EndRaid ?êÏÑú ?ºÍ¥Ñ ?Ä??
	UE_LOG(LogPDRaid, Verbose, TEXT("ProcessDeath: PC=%s -> raid inventory will be dropped on EndRaid"),
		*PC->GetName());
}

void APDGameMode::HandleSpectatorTransitionForDeath(APlayerController* DeadPC)
{
	APDPlayerController* DeadPDPC = Cast<APDPlayerController>(DeadPC);
	if (!DeadPDPC) return;

	APlayerController* InitialTarget = FindFirstAliveSpectateTarget(DeadPC);
	if (!InitialTarget)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("HandleSpectatorTransitionForDeath: PC=%s no alive target, skip"),
			*DeadPC->GetName());
		return;
	}

	DeadPDPC->StartSpectatingDeath(InitialTarget);
}

void APDGameMode::NotifyOthersWatchingFinalized(APlayerController* AffectedPC)
{
	if (!AffectedPC) return;
	UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APDPlayerController* Other = Cast<APDPlayerController>(It->Get());
		if (!Other || Other == AffectedPC) continue;
		Other->CycleSpectateIfTargetIs(AffectedPC);
	}
}

APlayerController* APDGameMode::FindFirstAliveSpectateTarget(APlayerController* ExcludePC) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || PC == ExcludePC) continue;

		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS) continue;
		if (PS->IsRaidDead() || PS->IsExtracted()) continue;
		if (!PC->GetPawn()) continue;

		return PC;
	}
	return nullptr;
}

float APDGameMode::GetCurrentRaidElapsedSeconds() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	return FMath::Max(0.f, Now - RaidStartServerTime);
}

int32 APDGameMode::CountInventoryItemQuantity(UPDInventoryComponent* Inventory) const
{
	if (!Inventory) return 0;
	int32 Total = 0;
	for (const FPDInventorySlot& Slot : Inventory->Items)
	{
		if (!Slot.IsEmpty())
		{
			Total += FMath::Max(0, Slot.Quantity);
		}
	}
	return Total;
}

void APDGameMode::CaptureInitialRaidSnapshotFor(APlayerController* PC)
{
	if (!PC) return;
	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	UPDInventoryComponent* Inventory = PS->GetInventoryComponent();
	const int32 InitialGold = Inventory ? Inventory->Gold : 0;
	const int32 InitialItemQty = CountInventoryItemQuantity(Inventory);
	PS->CaptureInitialRaidSnapshot(InitialGold, InitialItemQty);

	UE_LOG(LogPDRaid, Verbose, TEXT("CaptureInitialRaidSnapshot: PC=%s Gold=%d ItemQty=%d"),
		*PC->GetName(), InitialGold, InitialItemQty);
}

void APDGameMode::FinalizeRaidStatsOnDeath(APlayerController* DeadPC)
{
	if (!DeadPC) return;
	APDPlayerState* PS = DeadPC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	// ?¨Îßù?? ?∏Î≤§/Í≥®Îìú Î™®Îëê ?åÏã§ ??Delta ???úÏûë ?§ÎÉÖ???åÏàò.
	PS->SetSurvivalSeconds(GetCurrentRaidElapsedSeconds());
	PS->SetGoldDelta(-PS->GetInitialRaidGold());
	PS->SetItemDelta(-PS->GetInitialRaidItemQuantity());

	UE_LOG(LogPDRaid, Log, TEXT("FinalizeRaidStatsOnDeath: PC=%s Survive=%.1fs GoldDelta=%d ItemDelta=%d"),
		*DeadPC->GetName(),
		PS->GetRaidStats().SurvivalSeconds,
		PS->GetRaidStats().GoldDelta,
		PS->GetRaidStats().ItemDelta);
}

void APDGameMode::FinalizeRaidStatsOnExtraction(APlayerController* ExtractedPC)
{
	if (!ExtractedPC) return;
	APDPlayerState* PS = ExtractedPC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	UPDInventoryComponent* Inventory = PS->GetInventoryComponent();
	const int32 FinalGold = Inventory ? Inventory->Gold : 0;
	const int32 FinalItemQty = CountInventoryItemQuantity(Inventory);

	PS->SetSurvivalSeconds(GetCurrentRaidElapsedSeconds());
	PS->SetGoldDelta(FinalGold - PS->GetInitialRaidGold());
	PS->SetItemDelta(FinalItemQty - PS->GetInitialRaidItemQuantity());

	UE_LOG(LogPDRaid, Log, TEXT("FinalizeRaidStatsOnExtraction: PC=%s Survive=%.1fs GoldDelta=%d ItemDelta=%d"),
		*ExtractedPC->GetName(),
		PS->GetRaidStats().SurvivalSeconds,
		PS->GetRaidStats().GoldDelta,
		PS->GetRaidStats().ItemDelta);
}

void APDGameMode::CreditKillToShooter(AActor* Killer, APlayerController* Victim)
{
	if (!Killer || !Victim) return;

	// Killer ??Î≥¥ÌÜµ Í≥µÍ≤©?êÏùò Pawn. PlayerController Î°??òÏõê.
	APawn* KillerPawn = Cast<APawn>(Killer);
	APlayerController* KillerPC = KillerPawn ? Cast<APlayerController>(KillerPawn->GetController()) : Cast<APlayerController>(Killer);
	if (!KillerPC || KillerPC == Victim) return;

	APDPlayerState* KillerPS = KillerPC->GetPlayerState<APDPlayerState>();
	if (!KillerPS) return;

	KillerPS->AddKill();
	UE_LOG(LogPDRaid, Log, TEXT("CreditKillToShooter: Killer=%s Victim=%s Kills=%d"),
		*KillerPC->GetName(), *Victim->GetName(), KillerPS->GetRaidStats().Kills);
}

void APDGameMode::BuildRaidEndEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return;

	OutEntries.Reserve(GS->PlayerArray.Num());
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS) continue;

		FPDPlayerRaidEntryData Data;
		Data.PlayerName = PS->GetPlayerName();

		if (const APDPlayerState* PDPS = Cast<APDPlayerState>(PS))
		{
			Data.bSurvived = PDPS->IsExtracted();
			Data.Stats     = PDPS->GetRaidStats();
		}
		else
		{
			Data.bSurvived = false;
			Data.Stats     = FPDRaidStats{};
		}
		OutEntries.Add(MoveTemp(Data));
	}
}

void APDGameMode::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;

	UE_LOG(LogPDRaid, Log, TEXT("SetRaidState: %d -> %d"),
		static_cast<int32>(CurrentRaidState), static_cast<int32>(NewState));

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

void APDGameMode::NotifyPlayerReadyForTravel(APlayerController* PC)
{
	if (!PC || bTravelStarted) return;

	// Í≤∞ÏÇ∞ ?ÑÏ†Ø???®Í∏∞ ?ÑÏóê ?¥Î¶≠?ºÎ°ú ?§Ïñ¥?§Îäî ACK Ï∞®Îã®. EndRaid Í∞Ä ?∏Ï∂ú?òÏñ¥ Ended ?ÅÌÉú???åÎßå Î∞õÏùå.
	if (CurrentRaidState != ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("NotifyPlayerReadyForTravel: ignored, raid not Ended (state=%d) PC=%s"),
			static_cast<int32>(CurrentRaidState), *PC->GetName());
		return;
	}

	const bool bWasAdded = ReadyPlayersForTravel.AddUnique(PC) != INDEX_NONE;
	const int32 ReadyCount = ReadyPlayersForTravel.Num();
	const int32 TotalPlayers = GetGameState<AGameStateBase>() ? GetGameState<AGameStateBase>()->PlayerArray.Num() : 0;

	UE_LOG(LogPDRaid, Log, TEXT("NotifyPlayerReadyForTravel: PC=%s ack=%d/%d (newAck=%d)"),
		*PC->GetName(), ReadyCount, TotalPlayers, bWasAdded ? 1 : 0);

	if (AreAllPlayersReadyForTravel())
	{
		UE_LOG(LogPDRaid, Log, TEXT("NotifyPlayerReadyForTravel: all players ACK'd -> RequestBaseTravel"));
		RequestBaseTravel();
	}
}

bool APDGameMode::AreAllPlayersReadyForTravel() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return false;

	const int32 TotalPlayers = GS->PlayerArray.Num();
	if (TotalPlayers <= 0) return false;

	int32 ValidReadyCount = 0;
	for (const TWeakObjectPtr<APlayerController>& WeakPC : ReadyPlayersForTravel)
	{
		if (WeakPC.IsValid()) ++ValidReadyCount;
	}
	return ValidReadyCount >= TotalPlayers;
}

void APDGameMode::OnTravelTimeoutExpired()
{
	if (bTravelStarted) return;
	UE_LOG(LogPDRaid, Warning, TEXT("OnTravelTimeoutExpired: %.1fs gate expired, forcing ServerTravel"),
		TravelGateTimeoutSeconds);
	RequestBaseTravel();
}

void APDGameMode::RequestBaseTravel()
{
	if (bTravelStarted)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("RequestBaseTravel: already started, ignored"));
		return;
	}
	bTravelStarted = true;

	GetWorldTimerManager().ClearTimer(TravelTimeoutTimerHandle);

	UE_LOG(LogPDRaid, Log, TEXT("RequestBaseTravel: invoking seamless ServerTravel to BaseLevel"));

	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		GI->TravelToLevel(GI->GetBaseLevel(), /*bMarkBaseResetPending=*/false);
	}
}

int32 APDGameMode::CountAlivePlayers() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return 0;

	int32 Count = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		const APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (PS && !PS->IsRaidDead() && !PS->IsExtracted()) ++Count;
	}
	return Count;
}

int32 APDGameMode::CountExtractedPlayers() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return 0;

	int32 Count = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		const APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (PS && PS->IsExtracted()) ++Count;
	}
	return Count;
}

int32 APDGameMode::CountDeadPlayers() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return 0;

	int32 Count = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		const APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (PS && PS->IsRaidDead()) ++Count;
	}
	return Count;
}