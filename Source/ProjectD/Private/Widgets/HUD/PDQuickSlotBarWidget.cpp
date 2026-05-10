// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/HUD/PDQuickSlotBarWidget.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/HUD/PDQuickSlotItemWidget.h"

void UPDQuickSlotBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RebuildSlots();
}

void UPDQuickSlotBarWidget::SetSlotCount(int32 InSlotCount)
{
	const int32 NewCount = FMath::Max(0, InSlotCount);
	if (SlotCount == NewCount)
	{
		return;
	}

	SlotCount = NewCount;
	RebuildSlots();
}

void UPDQuickSlotBarWidget::SetSelectedIndex(int32 InIndex)
{
	const int32 NewIndex = (InIndex >= 0 && InIndex < SlotWidgets.Num()) ? InIndex : INDEX_NONE;
	if (SelectedIndex == NewIndex)
	{
		return;
	}

	SelectedIndex = NewIndex;
	ApplySelection();
}

void UPDQuickSlotBarWidget::SetSlotSpacing(float InSpacing)
{
	const float Clamped = FMath::Max(0.f, InSpacing);
	if (FMath::IsNearlyEqual(SlotSpacing, Clamped))
	{
		return;
	}

	SlotSpacing = Clamped;
	ApplySlotSpacing();
}

void UPDQuickSlotBarWidget::RebuildSlots()
{
	SlotWidgets.Reset();
	
	if (!SlotContainer || !SlotWidgetClass)
	{
		return;
	}

	SlotContainer->ClearChildren();

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UPDQuickSlotItemWidget* CreatedSlot = CreateWidget<UPDQuickSlotItemWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (!CreatedSlot)
		{
			continue;
		}

		SlotContainer->AddChild(CreatedSlot);
		SlotWidgets.Add(CreatedSlot);
	}

	ApplySlotSpacing();
	ApplySelection();
}

void UPDQuickSlotBarWidget::ApplySlotSpacing()
{
	if (!SlotContainer)
	{
		return;
	}

	// 첫 슬롯은 앞쪽 패딩 0, 두 번째부터 SlotSpacing 만큼 간격을 둠
	const TArray<UPanelSlot*>& PanelSlots = SlotContainer->GetSlots();
	for (int32 Index = 0; Index < PanelSlots.Num(); ++Index)
	{
		UPanelSlot* PanelSlot = PanelSlots[Index];
		if (!PanelSlot)
		{
			continue;
		}

		const float Lead = (Index == 0) ? 0.f : SlotSpacing;

		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
		{
			HBoxSlot->SetPadding(FMargin(Lead, 0.f, 0.f, 0.f));
		}
		else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(PanelSlot))
		{
			VBoxSlot->SetPadding(FMargin(0.f, Lead, 0.f, 0.f));
		}
	}
}

void UPDQuickSlotBarWidget::ApplySelection()
{
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (SlotWidgets[i])
		{
			SlotWidgets[i]->SetSelected(i == SelectedIndex);
		}
	}
}
