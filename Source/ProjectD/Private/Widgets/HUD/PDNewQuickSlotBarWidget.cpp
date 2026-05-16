#include "Widgets/HUD/PDNewQuickSlotBarWidget.h"

#include "Data/PDKeyIconDataAsset.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Items/PDQuickSlotComponent.h"
#include "Widgets/HUD/PDNewQuickSlotItemWidget.h"

void UPDNewQuickSlotBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BindQuickSlotComponent(FindQuickSlotComponent());
}

void UPDNewQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindQuickSlotComponent(FindQuickSlotComponent());
	CollectSlotWidgets();
	RefreshSlots();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleControlMappingsRebuilt);
		}
	}
}

void UPDNewQuickSlotBarWidget::NativeDestruct()
{
	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
	}

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleControlMappingsRebuilt);
		}
	}

	Super::NativeDestruct();
}

void UPDNewQuickSlotBarWidget::BindQuickSlotComponent(UPDQuickSlotComponent* InQuickSlotComponent)
{
	if (BoundQuickSlotComponent == InQuickSlotComponent)
	{
		RefreshSlots();
		return;
	}

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
	}

	BoundQuickSlotComponent = InQuickSlotComponent;

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->GridColumns = 6;
		BoundQuickSlotComponent->GridRows = 1;
		BoundQuickSlotComponent->InitializeQuickSlots();
		BoundQuickSlotComponent->OnQuickSlotsChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
	}

	CollectSlotWidgets();
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::CollectSlotWidgets()
{
	SlotWidgets.Reset();
	SlotWidgets.Add(Slot_0);
	SlotWidgets.Add(Slot_1);
	SlotWidgets.Add(Slot_2);
	SlotWidgets.Add(Slot_3);
	SlotWidgets.Add(Slot_4);
	SlotWidgets.Add(Slot_5);

	ApplyKeyBindings();
}

void UPDNewQuickSlotBarWidget::RefreshSlots()
{
	if (SlotWidgets.Num() == 0)
	{
		CollectSlotWidgets();
	}

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->InitializeQuickSlot(BoundQuickSlotComponent, Index);

		if (BoundQuickSlotComponent && BoundQuickSlotComponent->QuickSlotItems.IsValidIndex(Index))
		{
			SlotWidget->SetSlotData(BoundQuickSlotComponent->QuickSlotItems[Index]);
		}
		else
		{
			SlotWidget->ClearSlotData();
		}
	}

	ApplySelection(BoundQuickSlotComponent ? BoundQuickSlotComponent->GetSelectedIndex() : INDEX_NONE);
}

void UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged()
{
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::HandleSelectionChanged(int32 NewIndex)
{
	ApplySelection(NewIndex);
}

void UPDNewQuickSlotBarWidget::ApplySelection(int32 SelectedIndex)
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		if (UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index])
		{
			SlotWidget->SetSelected(Index == SelectedIndex);
		}
	}
}

void UPDNewQuickSlotBarWidget::ApplyKeyBindings()
{
	UPDKeyIconDataAsset* IconMap = KeyIconMap.LoadSynchronous();

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UPDNewQuickSlotItemWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		const UInputAction* Action = SlotInputActions.IsValidIndex(Index) ? SlotInputActions[Index] : nullptr;
		const FKey Key = FindKeyForAction(Action);

		UTexture2D* Icon = (IconMap && Key.IsValid()) ? IconMap->ResolveIcon(Key) : nullptr;
		SlotWidget->SetKeyBindingIcon(Icon);
	}
}

FKey UPDNewQuickSlotBarWidget::FindKeyForAction(const UInputAction* Action) const
{
	if (!Action || !InputMappingContext)
	{
		return FKey();
	}

	for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
	{
		if (Mapping.Action == Action)
		{
			return Mapping.Key;
		}
	}

	return FKey();
}

void UPDNewQuickSlotBarWidget::HandleControlMappingsRebuilt()
{
	ApplyKeyBindings();
}

UPDQuickSlotComponent* UPDNewQuickSlotBarWidget::FindQuickSlotComponent() const
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UPDQuickSlotComponent>();
	}
	return nullptr;
}