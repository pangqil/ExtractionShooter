#include "Core/PDGameMode.h"
#include "Core/PDGameState.h"
#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"
#include "Items/PDSecureContainerComponent.h"
#include "Core/PDPlayerState.h"
#include "Items/PDInventoryComponent.h"
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

	// Auto-start: 매 PostLogin 마다 디바운스 타이머 리셋. 마지막 합류 후
	// AutoStartDebounceSeconds 동안 새 합류가 없으면 StartRaid 1회 발사.
	// 화이트리스트 미포함 맵 (Base/Startup 등) 에서는 발사 안 됨.
	if (!bRaidStarted && IsCurrentMapAutoStartEnabled())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoStartDebounceHandle);
			World->GetTimerManager().SetTimer(
				AutoStartDebounceHandle,
				this,
				&APDGameMode::HandleAutoStartDebounceFired,
				AutoStartDebounceSeconds,
				false);
			UE_LOG(LogPDRaid, Verbose, TEXT("AutoStart: debounce armed (%.2fs) after PC=%s"),
				AutoStartDebounceSeconds, *GetNameSafe(NewPlayer));
		}
	}
}

void APDGameMode::HandleAutoStartDebounceFired()
{
	if (bRaidStarted) return;
	UE_LOG(LogPDRaid, Log, TEXT("AutoStart: debounce fired, calling StartRaid (NumPlayers=%d)"),
		GetWorld() ? GetWorld()->GetNumPlayerControllers() : -1);
	StartRaid();
}

bool APDGameMode::IsCurrentMapAutoStartEnabled() const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	// PIE prefix (UEDPIE_N_) 제거한 long package name 으로 비교.
	const FString CurrentMapName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());

	for (const TSoftObjectPtr<UWorld>& MapRef : AutoStartRaidLevels)
	{
		if (MapRef.IsNull()) continue;
		if (CurrentMapName == MapRef.ToSoftObjectPath().GetLongPackageName())
		{
			return true;
		}
	}
	return false;
}

void APDGameMode::StartRaid()
{
	if (bRaidStarted)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("StartRaid: already started, ignored"));
		return;
	}
	bRaidStarted = true;

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

		// Step 2-C: 초기 골드/아이템 수량 스냅샷 (사망/추출 시점에 Delta 계산용).
		CaptureInitialRaidSnapshotFor(PC);
		++PlayerCount;
	}

	SetRaidState(ERaidState::InProgress);
	UE_LOG(LogPDRaid, Log, TEXT("StartRaid: Participants=%d StartTime=%.2f"), PlayerCount, RaidStartServerTime);

	// Base→Raid 진입 연출: 각 클라에 라이드 시작 트랜지션 위젯 push (각자 로컬 재생 후 self-pop).
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APDPlayerController* PDPC = Cast<APDPlayerController>(It->Get()))
		{
			PDPC->Client_ShowRaidStartTransition(RaidZoneDisplayName);
		}
	}
}

void APDGameMode::TravelToRaidLevel(FName RaidMapName)
{
	if (RaidMapName.IsNone())
	{
		UE_LOG(LogPDRaid, Warning, TEXT("TravelToRaidLevel: RaidMapName is not set."));
		return;
	}

	// StartRaid 는 트래블 후 라이드 맵 GameMode 의 PostLogin 디바운스가 자동 호출
	// (bAutoStartRaidOnAllPlayersIn=true 인 BP). 트래블 전 호출은 GameMode/PS 폐기로 무효.
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

	// Step 2-C: 인벤토리가 stash 로 옮겨지기 전에 Delta 확정.
	FinalizeRaidStatsOnExtraction(PC);

	ProcessExtractionForPlayer(PC);

	// Step 2-B: 추출자를 관전하던 다른 PC 가 있으면 다음 생존자로 순환.
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

	// AutoStart 디바운스 도중 사망이 발생하면 디바운스 무시하고 즉시 StartRaid 강제.
	// 사망 발생 = 게임 진행 중 = race condition. StartRaid 안 거치면 EvaluateRaidEnd 가 Idle 가드에서 막힘.
	if (CurrentRaidState == ERaidState::Idle && !bRaidStarted)
	{
		UE_LOG(LogPDRaid, Log, TEXT("OnPlayerDied: state=Idle 사망 race 감지 → AutoStart 디바운스 취소 + 즉시 StartRaid."));
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoStartDebounceHandle);
		}
		StartRaid();
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
	else
	{
		UE_LOG(LogPDRaid, Warning, TEXT("OnPlayerDied: PC=%s GetPlayerState<APDPlayerState> returned NULL"), *PC->GetName());
	}

	UE_LOG(LogPDRaid, Log, TEXT("OnPlayerDied: PC=%s Killer=%s Alive=%d Dead=%d Extracted=%d"),
		*PC->GetName(),
		*GetNameSafe(Killer),
		CountAlivePlayers(), CountDeadPlayers(), CountExtractedPlayers());

	ProcessDeathForPlayer(PC);

	// Step 2-C: 사망자 결산 스탯 확정 + 킬러에게 Kill 크레딧.
	FinalizeRaidStatsOnDeath(PC);
	CreditKillToShooter(Killer, PC);

	// Step 2-B: 사망자는 첫 생존자 시점으로 관전 전환,
	// 죽은 PC를 보고 있던 다른 관전자들은 다음 생존자로 자동 순환.
	HandleSpectatorTransitionForDeath(PC);
	NotifyOthersWatchingFinalized(PC);

	// 배그식: 마지막 Alive 가 방금 죽었다면 남은 Downed 동료 BleedOut 기다리지 않고 즉시 처형.
	FinalizeStrandedDownedPlayers();

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
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: already Ended, ignored"));
		return;
	}
	SetRaidState(ERaidState::Ended);

	UE_LOG(LogPDRaid, Log, TEXT("EndRaid: bSuccess=%d Alive=%d Dead=%d Extracted=%d"),
		bSuccess ? 1 : 0,
		CountAlivePlayers(), CountDeadPlayers(), CountExtractedPlayers());

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();

	// 정산 마감: 추출자는 RequestExtraction 단계에서 이미 처리됨. 여기서는 사망자만 마저 처리.
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

		// 사망자: 인벤토리/RaidGold 손실. 영구 데이터(스태시/평판 등)만 저장.
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: PC=%s dead -> drop raid inventory, save persistent only"),
			*PC->GetName());
		SavePlayerStateToDisk(PC);
	}

	// SecureContainer 전역 규칙: 어느 누구도 추출 못 했으면 메모리 캐시 비움(다음 레이드 빈손).
	if (!bSuccess && GI)
	{
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: no survivors, clearing GI SecureContainer cache"));
		GI->SetSecureContainerItems({});
	}

	// BP 연출 훅 — 사운드/이펙트 등 서버 사이드 연출용. 결산 위젯 푸시는 아래 ClientRPC 로 일원화.
	OnRaidEnded(bSuccess);

	// 옵션 B: 모든 PC 에 결산 위젯 push ClientRPC.
	TArray<FPDPlayerRaidEntryData> Entries;
	BuildRaidEndEntries(Entries);
	const float Duration = GetCurrentRaidElapsedSeconds();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APDPlayerController* PDPC = Cast<APDPlayerController>(It->Get());
		if (!PDPC) continue;
		PDPC->Client_ShowRaidEndTransition(bSuccess, Entries, Duration);
	}

	// 모든 PC가 ACK하거나 TravelGateTimeoutSeconds 경과 시 ServerTravel. 둘 중 빠른 쪽이 발동.
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

	// SecureContainer 는 생존(추출) 시 다음 레이드로 이월.
	if (APawn* Pawn = PC->GetPawn())
	{
		if (UPDSecureContainerComponent* SecureContainer = Pawn->FindComponentByClass<UPDSecureContainerComponent>())
		{
			SecureContainer->SaveSecureContainer();
			UE_LOG(LogPDRaid, Log, TEXT("ProcessExtraction: PC=%s saved SecureContainer"), *PC->GetName());
		}
	}
}

void APDGameMode::ProcessDeathForPlayer(APlayerController* PC)
{
	if (!PC) return;

	// 사망자는 이체하지 않음(인벤/RaidGold 손실). 다른 영구 데이터는 EndRaid 에서 일괄 저장.
	UE_LOG(LogPDRaid, Log, TEXT("ProcessDeath: PC=%s -> raid inventory will be dropped on EndRaid"),
		*PC->GetName());
}

void APDGameMode::HandleSpectatorTransitionForDeath(APlayerController* DeadPC)
{
	APDPlayerController* DeadPDPC = Cast<APDPlayerController>(DeadPC);
	if (!DeadPDPC) return;

	APlayerController* InitialTarget = FindFirstAliveSpectateTarget(DeadPC);
	if (!InitialTarget)
	{
		UE_LOG(LogPDRaid, Log, TEXT("HandleSpectatorTransitionForDeath: PC=%s no alive target, skip"),
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

void APDGameMode::FinalizeStrandedDownedPlayers()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// LifeState 가 진짜 Alive 인 캐릭터를 카운트. Downed 는 따로 모음.
	int32 AliveCount = 0;
	TArray<APDCharacterBase*> StrandedDowned;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		APDCharacterBase* Char = Cast<APDCharacterBase>(PC->GetPawn());
		if (!Char) continue;

		switch (Char->GetLifeState())
		{
		case EPDLifeState::Alive:
			++AliveCount;
			break;
		case EPDLifeState::Downed:
			StrandedDowned.Add(Char);
			break;
		default:
			break;
		}
	}

	if (AliveCount > 0 || StrandedDowned.Num() == 0) return;

	UE_LOG(LogPDRaid, Log, TEXT("FinalizeStrandedDowned: no Alive remain, forcing %d Downed pawn(s) to die"),
		StrandedDowned.Num());

	// 각 HandleDeath 가 OnPlayerDied → FinalizeStrandedDownedPlayers 재귀 가능.
	// 이미 Dead 로 전환된 항목은 LifeState 가드로 스킵됨 (HandleDeath 내부).
	for (APDCharacterBase* DownedChar : StrandedDowned)
	{
		if (DownedChar && DownedChar->GetLifeState() == EPDLifeState::Downed)
		{
			DownedChar->HandleDeath(nullptr);
		}
	}
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

	UE_LOG(LogPDRaid, Log, TEXT("CaptureInitialRaidSnapshot: PC=%s Gold=%d ItemQty=%d"),
		*PC->GetName(), InitialGold, InitialItemQty);
}

void APDGameMode::FinalizeRaidStatsOnDeath(APlayerController* DeadPC)
{
	if (!DeadPC) return;
	APDPlayerState* PS = DeadPC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	// 사망자: 인벤/골드 모두 소실 → Delta 는 시작 스냅샷 음수.
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

	// Killer 는 보통 공격자의 Pawn. PlayerController 로 환원.
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

	// 결산 위젯이 뜨기 전에 클릭으로 들어오는 ACK 차단. EndRaid 가 호출되어 Ended 상태일 때만 받음.
	if (CurrentRaidState != ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Log, TEXT("NotifyPlayerReadyForTravel: ignored, raid not Ended (state=%d) PC=%s"),
			static_cast<int32>(CurrentRaidState), *PC->GetName());
		return;
	}

	const bool bWasAdded = ReadyPlayersForTravel.AddUnique(PC) != INDEX_NONE;
	const int32 ReadyCount = ReadyPlayersForTravel.Num();
	const int32 TotalPlayers = GetGameState<AGameStateBase>() ? GetGameState<AGameStateBase>()->PlayerArray.Num() : 0;

	// PS 측 ACK 플래그 셋팅 — 모든 클라가 위젯으로 누가 ACK 했는지 표시 가능.
	if (APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>())
	{
		PS->SetTravelReady(true);
	}

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
		UE_LOG(LogPDRaid, Log, TEXT("RequestBaseTravel: already started, ignored"));
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