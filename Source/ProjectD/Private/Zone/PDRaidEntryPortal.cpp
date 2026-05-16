#include "Zone/PDRaidEntryPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"

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
	
	const FPDPlayerData& Data = GI->GetPlayerData();
	GI->ConfirmRaidLoadout(Data.StashItems, Data.Gold);

	// 연출
	OnRaidEntryTriggered(PC);
}
