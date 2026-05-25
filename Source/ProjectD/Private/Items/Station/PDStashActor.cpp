#include "Items/Station/PDStashActor.h"

#include "Core/PDPlayerController.h"
#include "Items/Containers/PDStashComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

APDStashActor::APDStashActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	StashComponent = CreateDefaultSubobject<UPDStashComponent>(TEXT("StashComponent"));

	CurrentDoorAngle = ClosedDoorAngle;
	TargetDoorAngle = ClosedDoorAngle;
}

void APDStashActor::BeginPlay()
{
	Super::BeginPlay();

	SetDoorOpen(false, true);
}

void APDStashActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CurrentDoorAngle = ClosedDoorAngle;
	TargetDoorAngle = ClosedDoorAngle;
}

void APDStashActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentDoorAngle = FMath::FInterpTo(CurrentDoorAngle, TargetDoorAngle, DeltaSeconds, DoorInterpSpeed);

	if (FMath::IsNearlyEqual(CurrentDoorAngle, TargetDoorAngle, 0.1f))
	{
		CurrentDoorAngle = TargetDoorAngle;
		SetActorTickEnabled(false);
	}

}

bool APDStashActor::IsStationOpen(APDPlayerController* PlayerController) const
{
	return PlayerController &&
		PlayerController->IsStashInterfaceOpen() &&
		PlayerController->GetActiveStashComponent() == StashComponent;
}

void APDStashActor::OpenStation(APDPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->OpenStashInterface(StashComponent);
	}
}

void APDStashActor::CloseStation(APDPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->CloseStashInterface();
	}
}

void APDStashActor::SetDoorOpen(bool bOpen, bool bInstant)
{
	const float NewTargetDoorAngle = bOpen ? OpenDoorAngle : ClosedDoorAngle;
	const bool bTargetChanged = !FMath::IsNearlyEqual(TargetDoorAngle, NewTargetDoorAngle, 0.1f);
	TargetDoorAngle = NewTargetDoorAngle;

	if (bTargetChanged && !bInstant)
	{
		PlayDoorSound(bOpen);
	}

	if (bInstant)
	{
		CurrentDoorAngle = TargetDoorAngle;
		ApplyDoorAngle(CurrentDoorAngle);
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
}

void APDStashActor::PlayDoorSound(bool bOpen) const
{
	USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get();

	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), SoundVolumeMultiplier, SoundPitchMultiplier);
}

void APDStashActor::ApplyDoorAngle(float Angle)
{
}

void APDStashActor::BindStationClose(APDPlayerController* PlayerController)
{
	UnbindStationClose();
	BoundPlayerController = PlayerController;
	if (PlayerController)
	{
		PlayerController->OnStashInterfaceClosed.AddUObject(this, &APDStashActor::HandleContainerClosed);
	}
}

void APDStashActor::UnbindStationClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnStashInterfaceClosed.RemoveAll(this);
	}

	Super::UnbindStationClose();
}

void APDStashActor::HandleStationOpened(APDPlayerController*)
{
	OnStorageOpened.Broadcast(this);
}

void APDStashActor::HandleStationClosed()
{
	Super::HandleStationClosed();
	OnStorageClosed.Broadcast(this);
}

void APDStashActor::HandleContainerClosed(UPDStashComponent* ClosedStashComponent)
{
	if (ClosedStashComponent != StashComponent)
	{
		return;
	}

	HandleStationClosed();
}
