#include "Core/PDPlayerController.h"
#include "Characters/PDPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

#include "GameplayTag/PDGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Core/PDGameMode.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Input/PDInputComponent.h"

#include "InputCoreTypes.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Widgets/Crosshair/PDCrosshairWidget.h"
#include "Items/PDMarketComponent.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Widgets/HUD/PDHUDWidget.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDRootLayout.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogPDCharacter);

#include "Interfaces/PDInteractable.h"
#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDRifle.h"
#include "Weapons/PDShotgun.h"
#include "Weapons/PDSniper.h"

#include "Ping/PDPingSubsystem.h"


APDPlayerController::APDPlayerController()
{
	PrimaryActorTick.bCanEverTick=true;
}

void APDPlayerController::RequestExtraction()
{
	if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GM->RequestExtraction(this);
	}
}

void APDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateAimRotation();
	UpdateCrosshair();
}

void APDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	UPDInputComponent* PDIC = CastChecked<UPDInputComponent>(InputComponent);

	if (!InputConfig)
	{
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &APDPlayerController::ToggleInventory);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &APDPlayerController::TryInteract);
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &APDPlayerController::OnSwitchSlot1);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &APDPlayerController::OnSwitchSlot2);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &APDPlayerController::OnSwitchSlot3);
		InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &APDPlayerController::OnUseQuickSlot4);
		return;
	}

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Move,
		ETriggerEvent::Triggered, this, &APDPlayerController::OnMove);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Jump,
		ETriggerEvent::Started, this, &APDPlayerController::OnJump);

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_Inventory))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Inventory,
			ETriggerEvent::Started, this, &APDPlayerController::ToggleInventory);
	}
	else
	{
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &APDPlayerController::ToggleInventory);
	}

	if (InputConfig->FindNativeInputActionForTag(PDGameplayTags::Input_Interact))
	{
		PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
			ETriggerEvent::Started, this, &APDPlayerController::TryInteract);
	}
	else
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &APDPlayerController::TryInteract);
	}


	PDIC->BindAbilityActions(InputConfig, this, &APDPlayerController::OnAbilityInputPressed,
		&APDPlayerController::OnAbilityInputReleased);

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Fire,
		ETriggerEvent::Started, this, &APDPlayerController::OnFirePressed);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Fire,
		ETriggerEvent::Completed, this, &APDPlayerController::OnFireReleased);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Reload,
		ETriggerEvent::Started, this, &APDPlayerController::OnReload);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
		ETriggerEvent::Started, this, &APDPlayerController::OnInteract);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot1,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot1);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot2,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot2);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_SwitchSlot3,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot3);
	
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Zoom,
		ETriggerEvent::Started, this, &APDPlayerController::OnZoom);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_ToggleFireMode,
		ETriggerEvent::Started, this, &APDPlayerController::OnToggleFireMode);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_DropWeapon,
		ETriggerEvent::Started, this, &APDPlayerController::OnDropWeapon);

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &APDPlayerController::OnSwitchSlot1);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &APDPlayerController::OnSwitchSlot2);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &APDPlayerController::OnSwitchSlot3);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &APDPlayerController::OnUseQuickSlot4);
}

void APDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentMouseCursor = EMouseCursor::Default;

	if (CrosshairWidgetClass)
	{
		CrosshairWidget = CreateWidget<UPDCrosshairWidget>(this, CrosshairWidgetClass);
		if (CrosshairWidget)
			CrosshairWidget->AddToViewport(99);
	}
	
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		Ch->OnWeaponSwapped.AddDynamic(this, &APDPlayerController::OnWeaponChanged);

	CreateAndAddHUDWidget();
	
	if (RootLayoutClass)
	{
		RootLayoutInstance = CreateWidget<UPDRootLayout>(this, RootLayoutClass);
		if (RootLayoutInstance)
		{
			RootLayoutInstance->AddToViewport(5);
			if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(this))
			{
				UISubsystem->OnEffectiveUIStateChanged.AddUObject(this, &APDPlayerController::ApplyEffectiveUIState);
				UISubsystem->RegisterRootLayout(RootLayoutInstance);
			}
		}
	}
}

void APDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDInstance)
	{
		HUDInstance->Deactivate();
		HUDInstance->RemoveFromParent();
		HUDInstance = nullptr;
	}

	if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(this))
	{
		UISubsystem->OnEffectiveUIStateChanged.RemoveAll(this);
		UISubsystem->UnregisterRootLayout();
	}

	if (RootLayoutInstance)
	{
		RootLayoutInstance->RemoveFromParent();
		RootLayoutInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void APDPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HUDInstance)
	{
		UAbilitySystemComponent* ASC = InPawn
			? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn)
			: nullptr;
		HUDInstance->RebindToASC(ASC);
	}
}

void APDPlayerController::OnUnPossess()
{
	if (HUDInstance)
	{
		HUDInstance->RebindToASC(nullptr);
	}
	Super::OnUnPossess();
}

void APDPlayerController::CreateAndAddHUDWidget()
{
	if (!HUDClass) return;
	if (HUDInstance) return;

	HUDInstance = CreateWidget<UPDHUDWidget>(this, HUDClass);
	if (HUDInstance)
	{
		HUDInstance->AddToViewport(0);
		HUDInstance->Activate();
	}
}

void APDPlayerController::ApplyEffectiveUIState(EWidgetInputMode Mode)
{
	switch (Mode)
	{
	case EWidgetInputMode::Game:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
			bShowMouseCursor = false;
			if (CrosshairWidget)
			{
				CrosshairWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			break;
		}
	case EWidgetInputMode::GameAndMenu:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			if (CrosshairWidget)
			{
				CrosshairWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			break;
		}
	case EWidgetInputMode::Menu:
		{
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			if (CrosshairWidget)
			{
				CrosshairWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			break;
		}
	case EWidgetInputMode::Passive:
		break;
	}
}

void APDPlayerController::OnMove(const struct FInputActionValue& Value)
{
	if (IsGameplayInputBlockedByModalUI()) return;

	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput=Value.Get<FVector2D>();
	ControlledPawn->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	ControlledPawn->AddMovementInput(FVector::RightVector, MoveInput.X);
}

void APDPlayerController::OnJump()
{
	if (IsGameplayInputBlockedByModalUI()) return;

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}


void APDPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayTagContainer TagContainer(InputTag);
		ASC->CancelAbilities(&TagContainer);
	}
}


void APDPlayerController::OpenMarketInterface(UPDMarketComponent* MarketComponent)
{
	if (!MarketComponent)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("MarketComponent is not valid."));
		return;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	if (!MarketWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("MarketWidgetClass is not set."));
		return;
	}

	ActiveMarketComponent = MarketComponent;

	if (IsStashInterfaceOpen())
	{
		CloseStashInterface();
	}

	if (!InventoryWidgetInstance || !InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
		}
	}

	if (!MarketWidgetInstance || !MarketWidgetInstance->IsInViewport())
	{
		MarketWidgetInstance = CreateWidget<UPDMarketWidget>(this, MarketWidgetClass);
		if (MarketWidgetInstance)
		{
			MarketWidgetInstance->InitializeMarket(MarketComponent);
			MarketWidgetInstance->AddToViewport();
		}
	}
	else
	{
		MarketWidgetInstance->InitializeMarket(MarketComponent);
	}

	SetGameplayInputBlockedByModalUI(true, MarketWidgetInstance);
}

void APDPlayerController::CloseMarketInterface()
{
	if (MarketWidgetInstance && MarketWidgetInstance->IsInViewport())
	{
		MarketWidgetInstance->RemoveFromParent();
	}
	MarketWidgetInstance = nullptr;
	ActiveMarketComponent = nullptr;

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	InventoryWidgetInstance = nullptr;

	if (!IsStashInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool APDPlayerController::IsMarketInterfaceOpen() const
{
	return MarketWidgetInstance && MarketWidgetInstance->IsInViewport();
}

void APDPlayerController::OpenStashInterface()
{
	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	if (!StashWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("StashWidgetClass is not set."));
		return;
	}

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
	}

	if (!InventoryWidgetInstance || !InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
		}
	}

	if (!StashWidgetInstance || !StashWidgetInstance->IsInViewport())
	{
		StashWidgetInstance = CreateWidget<UPDStashWidget>(this, StashWidgetClass);
		if (StashWidgetInstance)
		{
			StashWidgetInstance->AddToViewport();
		}
	}

	SetGameplayInputBlockedByModalUI(true, StashWidgetInstance);
}

void APDPlayerController::CloseStashInterface()
{
	if (StashWidgetInstance && StashWidgetInstance->IsInViewport())
	{
		StashWidgetInstance->RemoveFromParent();
	}
	StashWidgetInstance = nullptr;

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	InventoryWidgetInstance = nullptr;

	if (!IsMarketInterfaceOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool APDPlayerController::IsStashInterfaceOpen() const
{
	return StashWidgetInstance && StashWidgetInstance->IsInViewport();
}


bool APDPlayerController::SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity)
{
	if (!ActiveMarketComponent || !IsMarketInterfaceOpen())
	{
		return false;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	UPDInventoryComponent* InventoryComponent = ControlledPawn->FindComponentByClass<UPDInventoryComponent>();
	if (!InventoryComponent)
	{
		return false;
	}

	return ActiveMarketComponent->SellInventorySlot(InventoryComponent, SlotIndex, Quantity);
}

void APDPlayerController::ToggleInventory()
{
	if (IsStashInterfaceOpen())
	{
		CloseStashInterface();
		return;
	}

	if (IsMarketInterfaceOpen())
	{
		CloseMarketInterface();
		return;
	}

	if (InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;

		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("InventoryWidgetClass is not set."));
		return;
	}

	InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(this, InventoryWidgetClass);
	if (!InventoryWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create inventory widget."));
		return;
	}

	InventoryWidgetInstance->AddToViewport();

	SetGameplayInputBlockedByModalUI(true, InventoryWidgetInstance);
}

void APDPlayerController::TryInteract()
{
	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->TryInteract();
	}
}

bool APDPlayerController::IsGameplayInputBlockedByModalUI() const
{
	return bIsGameplayInputBlockedByModalUI;
}

void APDPlayerController::SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus)
{
	if (bBlocked)
	{
		if (!bIsGameplayInputBlockedByModalUI)
		{
			bMouseCursorVisibleBeforeModalUI = bShowMouseCursor;
			bMouseClickEventsEnabledBeforeModalUI = bEnableClickEvents;
			bMouseOverEventsEnabledBeforeModalUI = bEnableMouseOverEvents;
		}

		bIsGameplayInputBlockedByModalUI = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);

		if (APawn* ControlledPawn = GetPawn())
		{
			if (UPawnMovementComponent* MovementComponent = ControlledPawn->GetMovementComponent())
			{
				MovementComponent->StopMovementImmediately();
			}
		}

		FInputModeGameAndUI InputMode;
		if (WidgetToFocus)
		{
			InputMode.SetWidgetToFocus(WidgetToFocus->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		DefaultMouseCursor = EMouseCursor::Default;
		CurrentMouseCursor = EMouseCursor::Default;
		return;
	}

	if (!bIsGameplayInputBlockedByModalUI)
	{
		return;
	}

	bIsGameplayInputBlockedByModalUI = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = bMouseCursorVisibleBeforeModalUI;
	bEnableClickEvents = bMouseClickEventsEnabledBeforeModalUI;
	bEnableMouseOverEvents = bMouseOverEventsEnabledBeforeModalUI;
}

void APDPlayerController::UpdateAimRotation()
{
	if (IsGameplayInputBlockedByModalUI()) return;

	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit)) return;

	FVector AimDirection=Hit.Location-ControlledPawn->GetActorLocation();
	AimDirection.Z=0.f;

	if (!AimDirection.IsNearlyZero())
	{
		ControlledPawn->SetActorRotation(AimDirection.Rotation());

		DrawDebugLine(GetWorld(), ControlledPawn->GetActorLocation(),
			ControlledPawn->GetActorLocation() + AimDirection.GetSafeNormal() * 200.f,
			FColor::Blue, false, 0.1f, 0, 2.f);
	}
}

void APDPlayerController::OnSwitchSlot1()
{
	UseQuickSlot(0);
}

void APDPlayerController::OnSwitchSlot2()
{
	UseQuickSlot(1);
}

void APDPlayerController::OnSwitchSlot3()
{
	UseQuickSlot(2);
}

void APDPlayerController::OnUseQuickSlot4()
{
	UseQuickSlot(3);
}

void APDPlayerController::OnZoom()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
			if (APDSniper* Sniper = Cast<APDSniper>(Weapon))
				Sniper->ToggleZoom();
}

void APDPlayerController::OnToggleFireMode()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
			if (APDRifle* Rifle = Cast<APDRifle>(Weapon))
				Rifle->ToggleFireMode();
}

void APDPlayerController::OnDropWeapon()
{
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		Ch->DropCurrentWeapon();
}

void APDPlayerController::UseQuickSlot(int32 SlotIndex)
{
	if (IsGameplayInputBlockedByModalUI())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (UPDQuickSlotComponent* QuickSlotComponent = ControlledPawn->FindComponentByClass<UPDQuickSlotComponent>())
	{
		if (QuickSlotComponent->UseQuickSlot(SlotIndex) && HUDInstance)
		{
			HUDInstance->RefreshNewQuickSlots();
		}
	}
}

void APDPlayerController::OnInteract()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	TArray<AActor*> OverlappingActors;
	ControlledPawn->GetOverlappingActors(OverlappingActors);

	AActor* ClosestInteractable = nullptr;
	float ClosestDist = FLT_MAX;

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor->Implements<UPDInteractable>()) continue;
		if (Actor->GetAttachParentActor() != nullptr) continue;

		float Dist = FVector::Dist(
			ControlledPawn->GetActorLocation(),
			Actor->GetActorLocation());

		if (Dist < ClosestDist)
		{
			ClosestDist = Dist;
			ClosestInteractable = Actor;
		}
	}

	if (ClosestInteractable)
		IPDInteractable::Execute_Interact(ClosestInteractable, ControlledPawn);
}

void APDPlayerController::OnFirePressed()
{	
	if (UPDPingSubsystem* PingSys = GetWorld()->GetSubsystem<UPDPingSubsystem>())
	{
		if (PingSys->IsPingActive()) return;
	}
	
	APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn());
	if (!Ch) return;
	APDWeaponBase* Weapon = Ch->GetCurrentWeapon();
	if (!Weapon) return;

	if (APDRifle* Rifle = Cast<APDRifle>(Weapon))
		Rifle->StartFire();
	else if (APDShotgun* Shotgun = Cast<APDShotgun>(Weapon))
	{
		if (Shotgun->IsReloading()) Shotgun->InterruptReloadAndFire();
		else Shotgun->Fire();
	}
	else
		Weapon->Fire();
}

void APDPlayerController::OnFireReleased()
{
	APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn());
	if (!Ch) return;
	if (APDRifle* Rifle = Cast<APDRifle>(Ch->GetCurrentWeapon()))
		Rifle->StopFire();
}

void APDPlayerController::OnReload()
{
	APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn());
	if (!Ch) return;
	if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
		Weapon->Reload();
}

void APDPlayerController::UpdateCrosshair()
{
	if (!CrosshairWidget) return;

	float MouseX, MouseY;
	GetMousePosition(MouseX, MouseY);

	float Spread = 0.f;
	if (APDPlayerCharacter* Ch = Cast<APDPlayerCharacter>(GetPawn()))
		if (APDWeaponBase* Weapon = Ch->GetCurrentWeapon())
			Spread = Weapon->GetCurrentRecoilSpread();

	CrosshairWidget->UpdateCrosshair(FVector2D(MouseX, MouseY), Spread);
}

void APDPlayerController::OnWeaponChanged(APDWeaponBase* NewWeapon, EWeaponSlot Slot)
{
	if (!CrosshairWidget || !NewWeapon) return;
	CrosshairWidget->SetCrosshairType(NewWeapon->GetWeaponType());
}

