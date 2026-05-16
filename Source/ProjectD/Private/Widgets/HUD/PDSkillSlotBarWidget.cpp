#include "Widgets/HUD/PDSkillSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "Widgets/HUD/PDSkillSlotWidget.h"

void UPDSkillSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildSlots();
}

void UPDSkillSlotBarWidget::SetSelectedIndex(int32 NewIndex)
{
	if (SelectedIndex == NewIndex)
	{
		return;
	}
	SelectedIndex = NewIndex;
	ApplySelection();
}

void UPDSkillSlotBarWidget::RebuildSlots()
{
	if (!SlotContainer && WidgetTree && !GetRootWidget())
	{
		UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotContainer"));
		WidgetTree->RootWidget = RootBox;
		SlotContainer = RootBox;
	}

	if (!SlotContainer || !SlotWidgetClass)
	{
		return;
	}

	SlotContainer->ClearChildren();
	SlotWidgets.Reset();

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UPDSkillSlotWidget* SlotWidget = CreateWidget<UPDSkillSlotWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotTextures(SelectedTexture, UnselectedTexture);

		UTexture2D* Icon = SkillIcons.IsValidIndex(Index) ? SkillIcons[Index].LoadSynchronous() : nullptr;
		SlotWidget->SetSkillIcon(Icon);

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

	ApplySelection();
}

void UPDSkillSlotBarWidget::ApplySelection()
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		if (UPDSkillSlotWidget* SlotWidget = SlotWidgets[Index])
		{
			SlotWidget->SetSelected(Index == SelectedIndex);
		}
	}
}