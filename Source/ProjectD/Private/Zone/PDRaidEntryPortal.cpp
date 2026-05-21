#include "Zone/PDRaidEntryPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Core/PDGameInstance.h"
#include "Core/PDGameMode.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"

APDRaidEntryPortal::APDRaidEntryPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = OverlapVolume;
}

void APDRaidEntryPortal::BeginPlay()
{
	Super::BeginPlay();
	OverlapVolume->OnComponentBeginOverlap.AddDynamic(this, &APDRaidEntryPortal::OnOverlapBegin);
}

void APDRaidEntryPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	APDPlayerController* PC = Cast<APDPlayerController>(Pawn->GetController());
	if (!PC) return;

	EnterRaid(PC);
}

void APDRaidEntryPortal::EnterRaid(APlayerController* PC)
{
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	APDPlayerState* PDPlayerState = PC ? PC->GetPlayerState<APDPlayerState>() : nullptr;
	if (!PDPlayerState) return;

	const FString SaveKey = GI->GetSaveKeyForController(PC);
	const FPDPlayerData Data = GI->LoadPlayerDataFromDisk(SaveKey, PC->IsLocalController());
	PDPlayerState->InitializePersistentData(Data);
	PDPlayerState->ConfirmRaidLoadout(Data.StashItems, Data.Gold);

	GI->SavePlayerDataToDisk(SaveKey, PDPlayerState->GetPersistentData());

	OnRaidEntryTriggered(PC);

	if (APDGameMode* GameMode = GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GameMode->TravelToRaidLevel(RaidLevelName);
	}
}
