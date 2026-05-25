#include "Items/World/PDLootBoxActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDLootComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

APDLootBoxActor::APDLootBoxActor()
{
	// Í±∞Î¶¨ Í∏∞Î∞ò ?òÏù¥?ºÏù¥???§ÌÖê??Í∞±Ïã† ???¥Îùº?¥Ïñ∏???úÍ∞Å ?®Í≥º??Tick ?ºÎ°ú Ï∂©Î∂Ñ.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// LootBox ÏΩòÌÖêÏ∏†Í? Î™®Îì† ?¥Îùº???ôÍ∏∞?îÎêò?ÑÎ°ù ?°ÌÑ∞ ?êÏ≤¥ Î¶¨ÌîåÎ¶¨Ï??¥ÏÖò ?úÏÑ±??
	bReplicates = true;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ConfigureInteractionCollision();

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(InteractionCollision);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LootComponent = CreateDefaultSubobject<UPDLootComponent>(TEXT("LootComponent"));
}

void APDLootBoxActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureInteractionCollision();

	// Overlay Î®∏Ìã∞Î¶¨Ïñº?Ä Í±∞Î¶¨ ?àÏóê ?§Ïñ¥?îÏùÑ ?åÎßå Î∂ÄÏ∞????úÎ°úÏΩ?ÎπÑÏö© ?åÌîº.
	// MID ????Î≤àÎßå ?ùÏÑ±?¥ÎëêÍ≥?Update ?êÏÑú ?åÎùºÎØ∏ÌÑ∞Îß?Í∞±Ïã†.
	if (BoxMesh && OutlineMaterial)
	{
		OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	}
}

void APDLootBoxActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Throttle ??HighlightUpdateInterval > 0 ?¥Î©¥ ?ÑÏ†Å?¥ÏÑú Í∑?Ï£ºÍ∏∞Î°úÎßå Í∞±Ïã†.
	if (HighlightUpdateInterval > 0.f)
	{
		HighlightAccumulator += DeltaSeconds;
		if (HighlightAccumulator < HighlightUpdateInterval) return;
		HighlightAccumulator = 0.f;
	}

	UpdateOutlineParams();
}

void APDLootBoxActor::UpdateOutlineParams()
{
	if (!BoxMesh || !OutlineMID) return;

	// Î°úÏª¨ PlayerController ??Pawn Í∏∞Ï??ºÎ°ú Í±∞Î¶¨ ?âÍ?. Î©Ä?∞Ìîå ??Í∞??¥ÎùºÍ∞Ä ?êÍ∏∞ ?úÏ†ê?ºÎ°ú Í≥ÑÏÇ∞.
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		BoxMesh->SetOverlayMaterial(nullptr);
		return;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
	const float MaxDistSq = HighlightMaxDistance * HighlightMaxDistance;

	if (DistSq > MaxDistSq)
	{
		// Î≤îÏúÑ Î∞???Overlay ?¥Ï†úÎ°??úÎ°úÏΩ??êÏ≤¥ ?úÍ±∞.
		BoxMesh->SetOverlayMaterial(nullptr);
		return;
	}

	// [MinDistance .. MaxDistance] ??[1 .. 0] ?åÌåå + Pow ÎπÑÏÑ†??
	const float Dist = FMath::Sqrt(DistSq);
	const float Range = FMath::Max(HighlightMaxDistance - HighlightMinDistance, KINDA_SMALL_NUMBER);
	const float LinearAlpha = FMath::Clamp((HighlightMaxDistance - Dist) / Range, 0.f, 1.f);
	const float Alpha = FMath::Pow(LinearAlpha, HighlightFalloffExponent);

	const float Intensity = FMath::Lerp(OutlineIntensityMin, OutlineIntensityMax, Alpha);
	const float Thickness = FMath::Lerp(OutlineThicknessMin, OutlineThicknessMax, Alpha);

	// Î≤îÏúÑ ?¨ÏßÑ????Overlay ?¨Î?Ï∞?(?¥Ï†Ñ Tick ?êÏÑú nullptr ?¥Ï†ú?êÏùÑ ???àÏùå).
	if (BoxMesh->GetOverlayMaterial() != OutlineMID)
	{
		BoxMesh->SetOverlayMaterial(OutlineMID);
	}

	OutlineMID->SetScalarParameterValue(IntensityParamName, Intensity);
	OutlineMID->SetScalarParameterValue(ThicknessParamName, Thickness);
}

void APDLootBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindLootClose();
	Super::EndPlay(EndPlayReason);
}

void APDLootBoxActor::Interact_Implementation(AActor* Interactor)
{
	APawn* InteractingPawn = Cast<APawn>(Interactor);
	if (!InteractingPawn) return;

	APDPlayerController* PlayerController = Cast<APDPlayerController>(InteractingPawn->GetController());
	if (!PlayerController) return;

	// ?ôÏùº Î∞ïÏä§ ?¨ÌÅ¥Î¶????´Í∏∞ ?†Í?.
	if (PlayerController->IsLootInterfaceOpen() && PlayerController->GetActiveLootComponent() == LootComponent)
	{
		PlayerController->CloseLootInterface();
		return;
	}

	PlayerController->OpenLootInterface(LootComponent);

	if (PlayerController->IsLootInterfaceOpen() && PlayerController->GetActiveLootComponent() == LootComponent)
	{
		BindLootClose(PlayerController);
		PlayInteractSound(true);
	}
}

void APDLootBoxActor::ConfigureInteractionCollision() const
{
	if (!InteractionCollision) return;

	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(true);
}

void APDLootBoxActor::PlayInteractSound(bool bOpen) const
{
	USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get();
	if (!Sound) return;

	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), SoundVolumeMultiplier);
}

void APDLootBoxActor::BindLootClose(APDPlayerController* PlayerController)
{
	if (!PlayerController) return;

	UnbindLootClose();
	BoundPlayerController = PlayerController;
	PlayerController->OnLootInterfaceClosed.AddUObject(this, &APDLootBoxActor::HandleLootInterfaceClosed);
}

void APDLootBoxActor::UnbindLootClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnLootInterfaceClosed.RemoveAll(this);
	}
	BoundPlayerController.Reset();
}

void APDLootBoxActor::HandleLootInterfaceClosed(UPDLootComponent* ClosedLootComponent)
{
	if (ClosedLootComponent != LootComponent) return;

	UnbindLootClose();
	PlayInteractSound(false);
}
