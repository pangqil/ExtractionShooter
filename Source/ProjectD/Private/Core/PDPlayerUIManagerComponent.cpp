#include "Core/PDPlayerUIManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/PDPlayerCharacter.h"
#include "Components/Widget.h"
#include "Core/PDPlayerController.h"
#include "Data/PDQuestComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDMarketComponent.h"
#include "Items/PDStashComponent.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Widgets/HUD/PDHUDWidget.h"
#include "Widgets/HUD/PDWorldMapWidget.h"
#include "Widgets/Inventory/PDInventoryWidget.h"
#include "Widgets/Inventory/PDMarketWidget.h"
#include "Widgets/Inventory/PDStashWidget.h"
#include "Widgets/PDNotificationWidget.h"
#include "Widgets/PDRootLayout.h"
#include "Widgets/Quest/PDQuestWindowWidget.h"

namespace
{
	bool ShouldShowCrosshairForController(const APDPlayerController* PC)
	{
		const APDPlayerCharacter* PlayerCharacter = PC ? Cast<APDPlayerCharacter>(PC->GetPawn()) : nullptr;
		return PlayerCharacter && PlayerCharacter->GetCurrentWeapon();
	}
}

UPDPlayerUIManagerComponent::UPDPlayerUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

APDPlayerController* UPDPlayerUIManagerComponent::GetPDController() const
{
	return Cast<APDPlayerController>(GetOwner());
}

void UPDPlayerUIManagerComponent::ConfigureLegacyWidgetClasses(
	TSubclassOf<UPDHUDWidget> InHUDClass,
	TSubclassOf<UPDRootLayout> InRootLayoutClass,
	TSubclassOf<UPDInventoryWidget> InInventoryWidgetClass,
	TSubclassOf<UPDStashWidget> InStashWidgetClass,
	TSubclassOf<UPDMarketWidget> InMarketWidgetClass,
	TSubclassOf<UUserWidget> InEquipmentModificationWidgetClass,
	TSubclassOf<UPDNotificationWidget> InNotificationWidgetClass,
	float InNotificationDuration,
	int32 InNotificationZOrder,
	TSubclassOf<UPDQuestWindowWidget> InQuestWindowWidgetClass,
	TSubclassOf<UPDWorldMapWidget> InWorldMapClass)
{
	if (!HUDClass) HUDClass = InHUDClass;
	if (!RootLayoutClass) RootLayoutClass = InRootLayoutClass;
	if (!InventoryWidgetClass) InventoryWidgetClass = InInventoryWidgetClass;
	if (!StashWidgetClass) StashWidgetClass = InStashWidgetClass;
	if (!MarketWidgetClass) MarketWidgetClass = InMarketWidgetClass;
	if (!EquipmentModificationWidgetClass) EquipmentModificationWidgetClass = InEquipmentModificationWidgetClass;
	if (!NotificationWidgetClass) NotificationWidgetClass = InNotificationWidgetClass;
	if (!QuestWindowWidgetClass) QuestWindowWidgetClass = InQuestWindowWidgetClass;
	if (!WorldMapClass) WorldMapClass = InWorldMapClass;

	NotificationDuration = InNotificationDuration;
	NotificationZOrder = InNotificationZOrder;
}

void UPDPlayerUIManagerComponent::InitializeUI(APDPlayerController* InOwnerController)
{
	APDPlayerController* PC = InOwnerController ? InOwnerController : GetPDController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	CreateAndAddHUDWidget();

	if (RootLayoutClass && !RootLayoutInstance)
	{
		RootLayoutInstance = CreateWidget<UPDRootLayout>(PC, RootLayoutClass);
		if (RootLayoutInstance)
		{
			RootLayoutInstance->AddToViewport(5);
			if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(PC))
			{
				UISubsystem->RegisterRootLayout(RootLayoutInstance);
				UISubsystem->OnEffectiveUIStateChanged.RemoveAll(this);
				UISubsystem->OnEffectiveUIStateChanged.AddUObject(this, &UPDPlayerUIManagerComponent::ApplyEffectiveUIState);
				ApplyEffectiveUIState(EWidgetInputMode::Game);
			}
		}
	}
}

void UPDPlayerUIManagerComponent::ShutdownUI(const EEndPlayReason::Type EndPlayReason)
{
	APDPlayerController* PC = GetPDController();

	CloseQuest();
	CloseMarket();
	CloseStash();
	CloseEquipmentModification();

	if (NotificationWidgetInstance)
	{
		NotificationWidgetInstance->RemoveFromParent();
		NotificationWidgetInstance = nullptr;
	}

	if (HUDInstance)
	{
		HUDInstance->Deactivate();
		HUDInstance->RemoveFromParent();
		HUDInstance = nullptr;
	}

	if (WorldMapInstance)
	{
		WorldMapInstance->RemoveFromParent();
		WorldMapInstance = nullptr;
	}

	if (PC)
	{
		if (UPDFrontendUISubsystem* UISubsystem = UPDFrontendUISubsystem::Get(PC))
		{
			UISubsystem->OnEffectiveUIStateChanged.RemoveAll(this);
			UISubsystem->UnregisterRootLayout();
		}
	}

	if (RootLayoutInstance)
	{
		RootLayoutInstance->RemoveFromParent();
		RootLayoutInstance = nullptr;
	}
}

void UPDPlayerUIManagerComponent::CreateAndAddHUDWidget()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController() || !HUDClass)
	{
		return;
	}

	if (!HUDInstance)
	{
		HUDInstance = CreateWidget<UPDHUDWidget>(PC, HUDClass);
	}

	AddWidgetToViewportIfNeeded(HUDInstance, 0);
	if (HUDInstance)
	{
		HUDInstance->Activate();
	}
}

void UPDPlayerUIManagerComponent::RebindHUDToASC(UAbilitySystemComponent* ASC)
{
	if (HUDInstance)
	{
		HUDInstance->RebindToASC(ASC);
	}
}

void UPDPlayerUIManagerComponent::ApplyEffectiveUIState(EWidgetInputMode Mode)
{
	if (HUDInstance)
	{
		HUDInstance->SetCrosshairVisible(
			Mode == EWidgetInputMode::Game && ShouldShowCrosshairForController(GetPDController()));
	}
}

void UPDPlayerUIManagerComponent::AddWidgetToViewportIfNeeded(UUserWidget* Widget, int32 ZOrder) const
{
	if (Widget && !Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}
}

void UPDPlayerUIManagerComponent::OpenMarket(UPDMarketComponent* MarketComponent)
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (!MarketComponent)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("MarketComponent is not valid."));
		return;
	}
	if (!MarketWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Market UI widget class is not set."));
		return;
	}

	ActiveMarketComponent = MarketComponent;
	CloseStash();
	CloseEquipmentModification();

	if (!MarketWidgetInstance)
	{
		MarketWidgetInstance = CreateWidget<UPDMarketWidget>(PC, MarketWidgetClass);
	}
	if (MarketWidgetInstance)
	{
		MarketWidgetInstance->InitializeMarket(MarketComponent);
		AddWidgetToViewportIfNeeded(MarketWidgetInstance);
	}

	SetGameplayInputBlockedByModalUI(true, MarketWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseMarket()
{
	if (MarketWidgetInstance)
	{
		MarketWidgetInstance->RemoveFromParent();
		MarketWidgetInstance = nullptr;
	}
	ActiveMarketComponent = nullptr;

	if (InventoryWidgetInstance && !IsStashOpen() && !IsEquipmentModificationOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	if (!IsStashOpen() && !IsQuestOpen() && !IsEquipmentModificationOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool UPDPlayerUIManagerComponent::IsMarketOpen() const
{
	return MarketWidgetInstance && MarketWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::OpenStash(UPDStashComponent* StashSource)
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (!InventoryWidgetClass || !StashWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Stash UI widget class is not set."));
		return;
	}
	if (!StashSource)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("OpenStash called with null StashSource."));
		return;
	}

	CloseMarket();
	CloseEquipmentModification();
	ActiveStashComponent = StashSource;

	if (!InventoryWidgetInstance)
	{
		InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(PC, InventoryWidgetClass);
	}
	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->SetActiveStashComponent(StashSource);
		AddWidgetToViewportIfNeeded(InventoryWidgetInstance);
	}

	if (!StashWidgetInstance)
	{
		StashWidgetInstance = CreateWidget<UPDStashWidget>(PC, StashWidgetClass);
	}
	if (StashWidgetInstance)
	{
		StashWidgetInstance->InitializeStash(StashSource);
		AddWidgetToViewportIfNeeded(StashWidgetInstance);
	}

	SetGameplayInputBlockedByModalUI(true, StashWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseStash()
{
	if (StashWidgetInstance)
	{
		StashWidgetInstance->RemoveFromParent();
		StashWidgetInstance = nullptr;
	}

	if (InventoryWidgetInstance && !IsMarketOpen() && !IsEquipmentModificationOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	ActiveStashComponent.Reset();

	if (!IsMarketOpen() && !IsQuestOpen() && !IsEquipmentModificationOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool UPDPlayerUIManagerComponent::IsStashOpen() const
{
	return StashWidgetInstance && StashWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::OpenEquipmentModification()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (IsEquipmentModificationOpen()) return;

	CloseStash();
	CloseMarket();
	CloseQuest();

	if (!EquipmentModificationWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Equipment modification UI widget class is not set."));
		return;
	}

	EquipmentModificationWidgetInstance = CreateWidget<UUserWidget>(PC, EquipmentModificationWidgetClass);
	if (!EquipmentModificationWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create equipment modification widget."));
		return;
	}

	AddWidgetToViewportIfNeeded(EquipmentModificationWidgetInstance);
	SetGameplayInputBlockedByModalUI(true, EquipmentModificationWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseEquipmentModification()
{
	if (EquipmentModificationWidgetInstance)
	{
		EquipmentModificationWidgetInstance->RemoveFromParent();
		EquipmentModificationWidgetInstance = nullptr;
	}

	if (InventoryWidgetInstance && !IsStashOpen() && !IsMarketOpen())
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	if (!IsStashOpen() && !IsMarketOpen() && !IsQuestOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool UPDPlayerUIManagerComponent::IsEquipmentModificationOpen() const
{
	return EquipmentModificationWidgetInstance && EquipmentModificationWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::OpenQuest()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;
	if (IsQuestOpen()) return;

	CloseStash();
	CloseMarket();
	CloseEquipmentModification();

	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	if (!QuestWindowWidgetClass)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("QuestWindowWidgetClass is not set."));
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	QuestWindowWidgetInstance = CreateWidget<UPDQuestWindowWidget>(PC, QuestWindowWidgetClass);
	if (!QuestWindowWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create quest window widget."));
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	QuestWindowWidgetInstance->InitializeQuestWindow(PC->GetPlayerQuestComponent(), PC->GetPlayerInventoryComponent());
	AddWidgetToViewportIfNeeded(QuestWindowWidgetInstance);
	SetGameplayInputBlockedByModalUI(true, QuestWindowWidgetInstance);
}

void UPDPlayerUIManagerComponent::CloseQuest()
{
	if (QuestWindowWidgetInstance)
	{
		QuestWindowWidgetInstance->RemoveFromParent();
		QuestWindowWidgetInstance = nullptr;
	}

	if (!IsStashOpen() && !IsMarketOpen() && !IsEquipmentModificationOpen())
	{
		SetGameplayInputBlockedByModalUI(false);
	}
}

bool UPDPlayerUIManagerComponent::IsQuestOpen() const
{
	return QuestWindowWidgetInstance && QuestWindowWidgetInstance->IsInViewport();
}

void UPDPlayerUIManagerComponent::ToggleQuest()
{
	if (IsQuestOpen())
	{
		CloseQuest();
		return;
	}
	OpenQuest();
}

void UPDPlayerUIManagerComponent::ToggleInventory()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;

	if (IsQuestOpen()) { CloseQuest(); return; }
	if (IsStashOpen()) { CloseStash(); return; }
	if (IsMarketOpen()) { CloseMarket(); return; }
	if (IsEquipmentModificationOpen()) { CloseEquipmentModification(); return; }

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

	InventoryWidgetInstance = CreateWidget<UPDInventoryWidget>(PC, InventoryWidgetClass);
	if (!InventoryWidgetInstance)
	{
		UE_LOG(LogPDCharacter, Warning, TEXT("Failed to create inventory widget."));
		return;
	}

	AddWidgetToViewportIfNeeded(InventoryWidgetInstance);
	SetGameplayInputBlockedByModalUI(true, InventoryWidgetInstance);
	PC->SetIgnoreMoveInput(false);
}

void UPDPlayerUIManagerComponent::ShowNotification(const FText& Message, float Duration)
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController() || Message.IsEmpty() || !NotificationWidgetClass)
	{
		return;
	}

	if (!NotificationWidgetInstance)
	{
		NotificationWidgetInstance = CreateWidget<UPDNotificationWidget>(PC, NotificationWidgetClass);
	}
	if (!NotificationWidgetInstance)
	{
		return;
	}

	AddWidgetToViewportIfNeeded(NotificationWidgetInstance, NotificationZOrder);
	NotificationWidgetInstance->ShowNotification(Message, Duration > 0.f ? Duration : NotificationDuration);
}

bool UPDPlayerUIManagerComponent::ShouldAllowMovementWhileUIOpen() const
{
	return InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport()
		&& !IsStashOpen()
		&& !IsMarketOpen()
		&& !IsEquipmentModificationOpen()
		&& !IsQuestOpen();
}

void UPDPlayerUIManagerComponent::SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus)
{
	APDPlayerController* PC = GetPDController();
	if (!PC)
	{
		return;
	}

	if (bBlocked)
	{
		if (!bIsGameplayInputBlockedByModalUI)
		{
			bMouseCursorVisibleBeforeModalUI = PC->bShowMouseCursor;
			bMouseClickEventsEnabledBeforeModalUI = PC->bEnableClickEvents;
			bMouseOverEventsEnabledBeforeModalUI = PC->bEnableMouseOverEvents;
		}

		bIsGameplayInputBlockedByModalUI = true;
		PC->OnFireReleased();
		const bool bAllowMovement = ShouldAllowMovementWhileUIOpen();
		PC->SetIgnoreMoveInput(!bAllowMovement);
		PC->SetIgnoreLookInput(true);

		if (!bAllowMovement)
		{
			if (APawn* ControlledPawn = PC->GetPawn())
			{
				if (UPawnMovementComponent* MovementComponent = ControlledPawn->GetMovementComponent())
				{
					MovementComponent->StopMovementImmediately();
				}
			}
		}

		FInputModeGameAndUI InputMode;
		if (WidgetToFocus)
		{
			InputMode.SetWidgetToFocus(WidgetToFocus->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		PC->DefaultMouseCursor = EMouseCursor::Default;
		PC->CurrentMouseCursor = EMouseCursor::Default;

		if (HUDInstance)
		{
			HUDInstance->SetCrosshairVisible(false);
		}
		return;
	}

	if (!bIsGameplayInputBlockedByModalUI)
	{
		return;
	}

	bIsGameplayInputBlockedByModalUI = false;
	PC->SetIgnoreMoveInput(false);
	PC->SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = bMouseCursorVisibleBeforeModalUI;
	PC->bEnableClickEvents = bMouseClickEventsEnabledBeforeModalUI;
	PC->bEnableMouseOverEvents = bMouseOverEventsEnabledBeforeModalUI;

	if (HUDInstance)
	{
		HUDInstance->SetCrosshairVisible(ShouldShowCrosshairForController(PC));
	}
}

void UPDPlayerUIManagerComponent::RefreshNewQuickSlots()
{
	if (HUDInstance)
	{
		HUDInstance->RefreshNewQuickSlots();
	}
}

void UPDPlayerUIManagerComponent::UpdateCrosshair(const FVector2D& MousePosition, float Spread)
{
	if (HUDInstance)
	{
		HUDInstance->UpdateCrosshair(MousePosition, Spread);
	}
}

void UPDPlayerUIManagerComponent::SetCrosshairType(APDWeaponBase* NewWeapon)
{
	if (!HUDInstance)
	{
		return;
	}

	if (!NewWeapon)
	{
		HUDInstance->SetCrosshairType(EWeaponType::None);
		HUDInstance->SetCrosshairVisible(false);
		return;
	}

	HUDInstance->SetCrosshairType(NewWeapon->GetWeaponType());
	if (!bIsGameplayInputBlockedByModalUI)
	{
		HUDInstance->SetCrosshairVisible(true);
	}
}

void UPDPlayerUIManagerComponent::ToggleWorldMap()
{
	APDPlayerController* PC = GetPDController();
	if (!PC || !PC->IsLocalController()) return;

	if (WorldMapInstance)
	{
		WorldMapInstance->RemoveFromParent();
		WorldMapInstance = nullptr;
		SetGameplayInputBlockedByModalUI(false);
		return;
	}

	if (!WorldMapClass) return;

	WorldMapInstance = CreateWidget<UPDWorldMapWidget>(PC, WorldMapClass);
	if (WorldMapInstance)
	{
		AddWidgetToViewportIfNeeded(WorldMapInstance, 10);
		SetGameplayInputBlockedByModalUI(true, WorldMapInstance);
	}
}
