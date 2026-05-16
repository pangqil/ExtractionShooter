#include "Widgets/HUD/PDNewQuickSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
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
	BuildFallbackWidget();
	BindQuickSlotComponent(FindQuickSlotComponent());
}

void UPDNewQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackWidget();
	BindQuickSlotComponent(FindQuickSlotComponent());
	RebuildSlotWidgets();
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
		BoundQuickSlotComponent->GridColumns = SlotCount;
		BoundQuickSlotComponent->GridRows = 1;
		BoundQuickSlotComponent->InitializeQuickSlots();
		BoundQuickSlotComponent->OnQuickSlotsChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
		BoundQuickSlotComponent->OnSelectionChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleSelectionChanged);
	}

	RebuildSlotWidgets();
	RefreshSlots();
}

void UPDNewQuickSlotBarWidget::RefreshSlots()
{
	if (SlotWidgets.Num() != SlotCount)
	{
		RebuildSlotWidgets();
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

void UPDNewQuickSlotBarWidget::BuildFallbackWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!SlotContainer)
	{
		if (UPanelWidget* ExistingPanel = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("SlotContainer"))))
		{
			SlotContainer = ExistingPanel;
		}
	}

	if (GetRootWidget())
	{
		return;
	}

	UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotContainer"));
	WidgetTree->RootWidget = RootBox;
	SlotContainer = RootBox;
}

void UPDNewQuickSlotBarWidget::RebuildSlotWidgets()
{
	BuildFallbackWidget();
	SlotWidgets.Reset();

	if (!SlotContainer)
	{
		return;
	}

	SlotContainer->ClearChildren();

	UClass* ClassToUse = SlotWidgetClass ? SlotWidgetClass.Get() : UPDNewQuickSlotItemWidget::StaticClass();

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UPDNewQuickSlotItemWidget* SlotWidget = CreateWidget<UPDNewQuickSlotItemWidget>(GetOwningPlayer(), ClassToUse);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->InitializeQuickSlot(BoundQuickSlotComponent, Index);
		SlotContainer->AddChild(SlotWidget);

		const bool bIsWeaponSlot = (Index < WeaponSlotCount);

		SlotWidget->SetSlotSize(bIsWeaponSlot ? WeaponSlotSize : ConsumableSlotSize);
		SlotWidget->SetSlotMaterials(
			bIsWeaponSlot ? WeaponSlotMaterial_Base : ConsumableSlotMaterial_Base,
			SlotMaterial_Selected);

		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(SlotWidget->Slot))
		{
			const float HalfSpacing = SlotSpacing * 0.5f;
			const bool bAtConsumableBoundary = (Index == WeaponSlotCount && Index > 0);
			const float LeftPadding = bAtConsumableBoundary ? (HalfSpacing + WeaponConsumableGroupGap) : HalfSpacing;

			HBoxSlot->SetPadding(FMargin(LeftPadding, 0.f, HalfSpacing, 0.f));
			HBoxSlot->SetHorizontalAlignment(HAlign_Center);
			HBoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		SlotWidgets.Add(SlotWidget);
	}

	ApplyKeyBindings();
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
