#include "Widgets/HUD/PDNewQuickSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
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
}

void UPDNewQuickSlotBarWidget::NativeDestruct()
{
	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
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
	}

	BoundQuickSlotComponent = InQuickSlotComponent;

	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->GridColumns = SlotCount;
		BoundQuickSlotComponent->GridRows = 1;
		BoundQuickSlotComponent->InitializeQuickSlots();
		BoundQuickSlotComponent->OnQuickSlotsChanged.AddUniqueDynamic(this, &UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged);
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
}

void UPDNewQuickSlotBarWidget::HandleQuickSlotsChanged()
{
	RefreshSlots();
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

		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(SlotWidget->Slot))
		{
			const float HalfSpacing = SlotSpacing * 0.5f;
			HBoxSlot->SetPadding(FMargin(HalfSpacing, 0.f, HalfSpacing, 0.f));
			HBoxSlot->SetHorizontalAlignment(HAlign_Center);
			HBoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		SlotWidgets.Add(SlotWidget);
	}
}

UPDQuickSlotComponent* UPDNewQuickSlotBarWidget::FindQuickSlotComponent() const
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UPDQuickSlotComponent>();
	}
	return nullptr;
}
