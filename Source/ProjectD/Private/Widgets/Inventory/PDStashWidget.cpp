#include "Widgets/Inventory/PDStashWidget.h"

#include "Algo/Sort.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/PDPlayerCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/UniformGridPanel.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/UniformGridSlot.h"
#include "GameFramework/Pawn.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDItemSlotTransfer.h"
#include "Items/PDQuickSlotComponent.h"
#include "Items/PDStashComponent.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"
#include "Widgets/Inventory/PDQuantityPopupWidget.h"

void UPDStashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindStashChanged();
	BindTabButtons();
	BindSortButtons();
	if (Button_UpgradeStash)
	{
		Button_UpgradeStash->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleUpgradeStashClicked);
	}
	SetSortOptionsVisible(false);
	RefreshStashGrid();
	UpdateTabButtonStyle();
}

void UPDStashWidget::NativeDestruct()
{
	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}
	ClearQuantityRequest();
	UnbindStashChanged();
	Super::NativeDestruct();
}

void UPDStashWidget::RefreshStashGrid()
{
	ResolveStashGridPanel();

	if (!StashGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDStashWidget: Stash grid widget was not found. Expected widget name: %s"), *StashGridWidgetName.ToString());
		return;
	}

	StashGridPanel->ClearChildren();
	// ScrollBox 안에서는 UniformGridPanel이 부모 높이에 맞춰 셀을 늘려 보일 수 있으므로,
	// 패널 쪽 슬롯 기본값을 명확히 고정한다. Padding은 GridSlot이 아니라 Panel 속성이다.
	StashGridPanel->SetSlotPadding(FMargin(0.f));
	StashGridPanel->SetMinDesiredSlotWidth(StashSlotWidth);
	StashGridPanel->SetMinDesiredSlotHeight(StashSlotHeight);

	if (!StashSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDStashWidget: StashSlotWidgetClass is not set. Set it to WBP_InventorySlot in WBP_Stash Class Defaults."));
		return;
	}

	UPDStashComponent* StashComponent = FindStashComponent();

	// 창고 업그레이드가 반영되도록 실제 컴포넌트의 열/행을 우선 사용한다.
	const int32 Columns = FMath::Max(1, StashComponent ? StashComponent->GridColumns : FallbackGridColumns);
	const int32 Rows = FMath::Max(1, StashComponent ? StashComponent->GridRows : FallbackGridRows);
	const int32 SlotCount = Columns * Rows;

	TArray<int32> DisplaySlotIndices;
	if (StashComponent)
	{
		// 현재 탭 아이템을 먼저 모아 보여주고, 남는 칸은 실제 빈 스태시 슬롯 인덱스로 매핑한다.
		// 그래야 빈 칸에 드롭해도 INDEX_NONE이 아니라 실제 스태시 슬롯으로 이동된다.
		for (int32 RealSlotIndex = 0; RealSlotIndex < StashComponent->StashItems.Num(); ++RealSlotIndex)
		{
			const FPDInventorySlot& StashSlotData = StashComponent->StashItems[RealSlotIndex];
			if (!StashSlotData.IsEmpty() && DoesSlotMatchCurrentFilter(StashSlotData))
			{
				DisplaySlotIndices.Add(RealSlotIndex);
			}
		}

		SortDisplaySlotIndices(DisplaySlotIndices, StashComponent);

		for (int32 RealSlotIndex = 0; RealSlotIndex < StashComponent->StashItems.Num(); ++RealSlotIndex)
		{
			const FPDInventorySlot& StashSlotData = StashComponent->StashItems[RealSlotIndex];
			if (StashSlotData.IsEmpty())
			{
				DisplaySlotIndices.Add(RealSlotIndex);
			}
		}
	}

	for (int32 DisplayIndex = 0; DisplayIndex < SlotCount; ++DisplayIndex)
	{
		UUserWidget* CreatedSlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), StashSlotWidgetClass);
		if (!CreatedSlotWidget)
		{
			continue;
		}

		if (UPDInventorySlotWidget* StashSlotWidget = Cast<UPDInventorySlotWidget>(CreatedSlotWidget))
		{
			StashSlotWidget->SetSlotContainerType(EPDItemContainerType::Stash);
			StashSlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotLeftClicked);
			StashSlotWidget->OnSlotItemDropped.AddUniqueDynamic(this, &UPDStashWidget::HandleStashSlotItemDropped);

			if (StashComponent && DisplaySlotIndices.IsValidIndex(DisplayIndex))
			{
				const int32 RealSlotIndex = DisplaySlotIndices[DisplayIndex];
				if (StashComponent->StashItems.IsValidIndex(RealSlotIndex) && !StashComponent->StashItems[RealSlotIndex].IsEmpty())
				{
					StashSlotWidget->SetSlotData(StashComponent->StashItems[RealSlotIndex], RealSlotIndex);
				}
				else
				{
					StashSlotWidget->ClearSlotData(RealSlotIndex);
				}
			}
			else
			{
				StashSlotWidget->ClearSlotData(INDEX_NONE);
			}
		}

		USizeBox* SlotSizeBox = WidgetTree ? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()) : nullptr;
		if (SlotSizeBox)
		{
			SlotSizeBox->SetWidthOverride(StashSlotWidth);
			SlotSizeBox->SetHeightOverride(StashSlotHeight);
			SlotSizeBox->SetMinDesiredWidth(StashSlotWidth);
			SlotSizeBox->SetMinDesiredHeight(StashSlotHeight);

			if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SlotSizeBox->AddChild(CreatedSlotWidget)))
			{
				// 슬롯 BP를 120x120 셀 중앙에 작은 DesiredSize로 배치하지 말고, 셀을 꽉 채우게 한다.
				SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
				SizeBoxSlot->SetPadding(FMargin(0.f));
			}
		}

		UWidget* GridChildWidget = SlotSizeBox ? static_cast<UWidget*>(SlotSizeBox) : static_cast<UWidget*>(CreatedSlotWidget);
		UUniformGridSlot* GridSlot = StashGridPanel->AddChildToUniformGrid(GridChildWidget, DisplayIndex / Columns, DisplayIndex % Columns);
		if (GridSlot)
		{
			// Fill로 두면 ScrollBox/UniformGridPanel의 남는 높이를 받아 슬롯이 세로로 늘어날 수 있다.
			// SizeBox가 가진 120x120 크기만 사용하도록 좌상단 정렬로 고정한다.
			GridSlot->SetHorizontalAlignment(HAlign_Left);
			GridSlot->SetVerticalAlignment(VAlign_Top);
		}
	}

	UpdateTabButtonStyle();
}

void UPDStashWidget::SetStashFilterTab(EPDItemFilterTab NewFilterTab)
{
	if (CurrentFilterTab == NewFilterTab)
	{
		UpdateTabButtonStyle();
		return;
	}

	CurrentFilterTab = NewFilterTab;
	CurrentSortMode = EPDItemSortMode::None;
	SetSortOptionsVisible(false);
	RefreshStashGrid();
}

void UPDStashWidget::SetStashSortMode(EPDItemSortMode NewSortMode)
{
	if (CurrentSortMode == NewSortMode)
	{
		SetSortOptionsVisible(false);
		return;
	}

	CurrentSortMode = NewSortMode;
	SetSortOptionsVisible(false);
	RefreshStashGrid();
}

void UPDStashWidget::BindTabButtons()
{
	if (Button_Equipment)
	{
		Button_Equipment->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleEquipmentTabClicked);
	}

	if (Button_Consumable)
	{
		Button_Consumable->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleConsumableTabClicked);
	}

	if (Button_Misc)
	{
		Button_Misc->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleMiscTabClicked);
	}
}

void UPDStashWidget::BindSortButtons()
{
	if (Button_Sort)
	{
		Button_Sort->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortButtonClicked);
	}

	if (Button_SortByName)
	{
		Button_SortByName->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByNameClicked);
	}

	if (Button_SortByType)
	{
		Button_SortByType->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByTypeClicked);
	}

	if (Button_SortTab_Name)
	{
		Button_SortTab_Name->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByNameClicked);
	}

	if (Button_SortTab_Type)
	{
		Button_SortTab_Type->OnClicked.AddUniqueDynamic(this, &UPDStashWidget::HandleSortByTypeClicked);
	}
}

void UPDStashWidget::UpdateTabButtonStyle()
{
	const FLinearColor SelectedColor(0.15f, 0.85f, 0.15f, 1.0f);
	const FLinearColor NormalColor(0.02f, 0.02f, 0.02f, 0.85f);

	const int32 MaxSlots = GetStashDisplaySlotCount();
	const int32 EquipmentUsedSlots = CountOccupiedStashSlotsByType(EPDItemType::Equipment);
	const int32 ConsumableUsedSlots = CountOccupiedStashSlotsByType(EPDItemType::Consumable);
	const int32 MiscUsedSlots = CountOccupiedStashSlotsByType(EPDItemType::Misc);

	if (Button_Equipment)
	{
		Button_Equipment->SetBackgroundColor(CurrentFilterTab == EPDItemFilterTab::Equipment ? SelectedColor : NormalColor);
		SetTabButtonLabel(Button_Equipment, FText::FromString(TEXT("장비")), EquipmentUsedSlots, MaxSlots);
	}

	if (Button_Consumable)
	{
		Button_Consumable->SetBackgroundColor(CurrentFilterTab == EPDItemFilterTab::Consumable ? SelectedColor : NormalColor);
		SetTabButtonLabel(Button_Consumable, FText::FromString(TEXT("소모")), ConsumableUsedSlots, MaxSlots);
	}

	if (Button_Misc)
	{
		Button_Misc->SetBackgroundColor(CurrentFilterTab == EPDItemFilterTab::Misc ? SelectedColor : NormalColor);
		SetTabButtonLabel(Button_Misc, FText::FromString(TEXT("기타")), MiscUsedSlots, MaxSlots);
	}
}

int32 UPDStashWidget::CountOccupiedStashSlotsByType(EPDItemType ItemType) const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	if (!StashComponent)
	{
		return 0;
	}

	int32 UsedSlotCount = 0;
	for (const FPDInventorySlot& StashSlotData : StashComponent->StashItems)
	{
		if (!StashSlotData.IsEmpty() && StashSlotData.ItemData.ItemType == ItemType)
		{
			++UsedSlotCount;
		}
	}

	return UsedSlotCount;
}

int32 UPDStashWidget::GetStashDisplaySlotCount() const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	const int32 Columns = FMath::Max(1, StashComponent ? StashComponent->GridColumns : FallbackGridColumns);
	const int32 Rows = FMath::Max(1, StashComponent ? StashComponent->GridRows : FallbackGridRows);
	return Columns * Rows;
}

void UPDStashWidget::SetTabButtonLabel(UButton* TargetButton, const FText& BaseLabel, int32 UsedSlots, int32 MaxSlots) const
{
	if (!TargetButton)
	{
		return;
	}

	if (UTextBlock* ButtonText = Cast<UTextBlock>(TargetButton->GetContent()))
	{
		ButtonText->SetText(FText::FromString(FString::Printf(TEXT("%s (%d/%d)"), *BaseLabel.ToString(), UsedSlots, FMath::Max(1, MaxSlots))));
	}
}

bool UPDStashWidget::DoesSlotMatchCurrentFilter(const FPDInventorySlot& StashSlotData) const
{
	return !StashSlotData.IsEmpty() && DoesItemTypeMatchCurrentFilter(StashSlotData.ItemData.ItemType);
}

bool UPDStashWidget::DoesItemTypeMatchCurrentFilter(EPDItemType ItemType) const
{
	if (CurrentFilterTab == EPDItemFilterTab::Equipment)
	{
		return ItemType == EPDItemType::Equipment;
	}

	if (CurrentFilterTab == EPDItemFilterTab::Consumable)
	{
		return ItemType == EPDItemType::Consumable;
	}

	if (CurrentFilterTab == EPDItemFilterTab::Misc)
	{
		return ItemType == EPDItemType::Misc;
	}

	return false;
}

bool UPDStashWidget::CanAcceptDropForCurrentFilter(const UPDInventoryDragDropOperation* DragOperation) const
{
	return DragOperation
		&& DragOperation->IsValidPayload()
		&& !DragOperation->SlotData.IsEmpty()
		&& DoesItemTypeMatchCurrentFilter(DragOperation->SlotData.ItemData.ItemType);
}

void UPDStashWidget::SortDisplaySlotIndices(TArray<int32>& DisplaySlotIndices, const UPDStashComponent* StashComponent) const
{
	if (!StashComponent || CurrentSortMode == EPDItemSortMode::None)
	{
		return;
	}

	Algo::Sort(DisplaySlotIndices, [this, StashComponent](int32 LeftIndex, int32 RightIndex)
	{
		const FPDInventorySlot& LeftSlot = StashComponent->StashItems[LeftIndex];
		const FPDInventorySlot& RightSlot = StashComponent->StashItems[RightIndex];

		if (CurrentSortMode == EPDItemSortMode::Type)
		{
			const uint8 LeftType = static_cast<uint8>(LeftSlot.ItemData.ItemType);
			const uint8 RightType = static_cast<uint8>(RightSlot.ItemData.ItemType);
			if (LeftType != RightType)
			{
				return LeftType < RightType;
			}
		}

		const FString LeftName = LeftSlot.ItemData.DisplayName.IsEmpty() ? LeftSlot.ItemData.ItemID.ToString() : LeftSlot.ItemData.DisplayName.ToString();
		const FString RightName = RightSlot.ItemData.DisplayName.IsEmpty() ? RightSlot.ItemData.ItemID.ToString() : RightSlot.ItemData.DisplayName.ToString();
		const int32 NameCompare = LeftName.Compare(RightName, ESearchCase::IgnoreCase);
		if (NameCompare != 0)
		{
			return NameCompare < 0;
		}

		return LeftIndex < RightIndex;
	});
}

void UPDStashWidget::SetSortOptionsVisible(bool bVisible)
{
	UWidget* SortPanel = Panel_SortTabs ? Panel_SortTabs.Get() : Panel_SortOptions.Get();
	if (SortPanel)
	{
		SortPanel->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPDStashWidget::ToggleSortOptions()
{
	UWidget* SortPanel = Panel_SortTabs ? Panel_SortTabs.Get() : Panel_SortOptions.Get();
	if (SortPanel)
	{
		SetSortOptionsVisible(SortPanel->GetVisibility() != ESlateVisibility::Visible);
	}
}

void UPDStashWidget::HandleEquipmentTabClicked()
{
	SetStashFilterTab(EPDItemFilterTab::Equipment);
}

void UPDStashWidget::HandleConsumableTabClicked()
{
	SetStashFilterTab(EPDItemFilterTab::Consumable);
}

void UPDStashWidget::HandleMiscTabClicked()
{
	SetStashFilterTab(EPDItemFilterTab::Misc);
}

void UPDStashWidget::HandleSortButtonClicked()
{
	ToggleSortOptions();
}

void UPDStashWidget::HandleSortByNameClicked()
{
	SetStashSortMode(EPDItemSortMode::Name);
}

void UPDStashWidget::HandleSortByTypeClicked()
{
	SetStashSortMode(EPDItemSortMode::Type);
}

void UPDStashWidget::HandleUpgradeStashClicked()
{
	RequestStashUpgrade();
}

EPDStashUpgradeResult UPDStashWidget::RequestStashUpgrade()
{
	UPDStashComponent* StashComponent = FindStashComponent();
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();

	if (!StashComponent)
	{
		BP_OnStashUpgradeFailed(EPDStashUpgradeResult::InvalidConfig);
		return EPDStashUpgradeResult::InvalidConfig;
	}

	const EPDStashUpgradeResult Result = StashComponent->UpgradeStash(InventoryComponent);
	if (Result == EPDStashUpgradeResult::Success)
	{
		RefreshStashGrid();
		UpdateTabButtonStyle();
		BP_OnStashUpgradeSucceeded(StashComponent->CurrentUpgradeLevel, StashComponent->GridRows);
	}
	else
	{
		BP_OnStashUpgradeFailed(Result);
	}

	return Result;
}

void UPDStashWidget::ResolveStashGridPanel()
{
	if (StashGridPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* FoundWidget = WidgetTree->FindWidget(StashGridWidgetName))
	{
		StashGridPanel = Cast<UUniformGridPanel>(FoundWidget);
	}

	if (!StashGridPanel)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!StashGridPanel)
			{
				StashGridPanel = Cast<UUniformGridPanel>(Widget);
			}
		});
	}
}

void UPDStashWidget::InitializeStash(UPDStashComponent* InStashComponent)
{
	if (TargetStashComponent == InStashComponent)
	{
		return;
	}

	UnbindStashChanged();
	TargetStashComponent = InStashComponent;
	BindStashChanged();
	RefreshStashGrid();
}

UPDStashComponent* UPDStashWidget::FindStashComponent() const
{
	return TargetStashComponent;
}

UPDInventoryComponent* UPDStashWidget::FindInventoryComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
}

UPDQuickSlotComponent* UPDStashWidget::FindQuickSlotComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDQuickSlotComponent>();
	}

	return nullptr;
}

const FPDInventorySlot* UPDStashWidget::FindStashSlot(int32 SlotIndex) const
{
	const UPDStashComponent* StashComponent = FindStashComponent();
	return StashComponent && StashComponent->StashItems.IsValidIndex(SlotIndex) ? &StashComponent->StashItems[SlotIndex] : nullptr;
}

const FPDInventorySlot* UPDStashWidget::FindSourceSlot(EPDItemContainerType SourceContainerType, int32 SlotIndex) const
{
	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (const UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
			return InventoryComponent->Items.IsValidIndex(SlotIndex) ? &InventoryComponent->Items[SlotIndex] : nullptr;
		}
		return nullptr;
	case EPDItemContainerType::Stash:
		return FindStashSlot(SlotIndex);
	case EPDItemContainerType::QuickSlot:
		if (const UPDQuickSlotComponent* QuickSlotComponent = FindQuickSlotComponent())
		{
			return QuickSlotComponent->QuickSlotItems.IsValidIndex(SlotIndex) ? &QuickSlotComponent->QuickSlotItems[SlotIndex] : nullptr;
		}
		return nullptr;
	default:
		return nullptr;
	}
}

void UPDStashWidget::HandleStashSlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex)
{
	if (!SlotWidget || !SlotWidget->WasLastClickWithControl() || SlotWidget->GetSlotData().IsEmpty())
	{
		return;
	}

	const FPDInventorySlot& SlotData = SlotWidget->GetSlotData();
	if (SlotData.Quantity <= 1 || FMath::Max(1, SlotData.ItemData.MaxStack) <= 1)
	{
		TakeStashSlotQuantity(ClickedSlotIndex, 1);
		return;
	}

	const FText DisplayName = SlotData.ItemData.DisplayName.IsEmpty() ? FText::FromName(SlotData.ItemData.ItemID) : SlotData.ItemData.DisplayName;
	OpenQuantityPopup(ClickedSlotIndex, SlotData.Quantity, DisplayName);
}

void UPDStashWidget::HandleStashSlotItemDropped(UPDInventorySlotWidget* SlotWidget, int32 TargetSlotIndex, UPDInventoryDragDropOperation* DragOperation)
{
	if (!SlotWidget || !CanAcceptDropForCurrentFilter(DragOperation) || TargetSlotIndex == INDEX_NONE)
	{
		return;
	}

	// Stash should only store real inventory/stash items. Ignore quick-slot references.
	if (DragOperation->SourceContainerType == EPDItemContainerType::QuickSlot)
	{
		return;
	}

	if (DragOperation->SourceContainerType == EPDItemContainerType::Stash)
	{
		ExecuteStashSlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
		return;
	}

	int32 MaxQuantity = 0;
	FText Title;
	if (ShouldOpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title))
	{
		OpenTransferQuantityPopup(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, MaxQuantity, Title);
		return;
	}

	ExecuteStashSlotTransfer(DragOperation->SourceContainerType, DragOperation->SourceSlotIndex, TargetSlotIndex, DragOperation->SlotData.Quantity);
}

void UPDStashWidget::TakeStashSlotQuantity(int32 SlotIndex, int32 Quantity)
{
	UPDInventoryComponent* InventoryComponent = FindInventoryComponent();
	UPDStashComponent* StashComponent = FindStashComponent();

	if (!InventoryComponent || !StashComponent || Quantity <= 0)
	{
		return;
	}

	// 무기 아이템이고 해당 슬롯이 비어있으면 인벤토리 우회하여 즉시 장착.
	if (const FPDInventorySlot* SourceSlot = FindStashSlot(SlotIndex))
	{
		if (SourceSlot->ItemData.WeaponClass)
		{
			if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(GetOwningPlayerPawn()))
			{
				if (PlayerCharacter->TryAutoEquipWeaponSlot(*SourceSlot))
				{
					StashComponent->StashItems[SlotIndex].Quantity -= 1;
					if (StashComponent->StashItems[SlotIndex].Quantity <= 0)
					{
						StashComponent->StashItems[SlotIndex].Clear();
					}
					StashComponent->OnStashChanged.Broadcast();
					return;
				}
			}
		}
	}

	StashComponent->TakeStashSlotQuantity(InventoryComponent, SlotIndex, Quantity);
}


void UPDStashWidget::ExecuteStashSlotTransfer(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return;
	}

	UPDStashComponent* StashComponent = FindStashComponent();
	if (!StashComponent)
	{
		return;
	}

	switch (SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		if (UPDInventoryComponent* InventoryComponent = FindInventoryComponent())
		{
			StashComponent->StoreInventorySlotQuantityToSlot(InventoryComponent, SourceSlotIndex, TargetSlotIndex, Quantity);
		}
		break;
	case EPDItemContainerType::Stash:
		StashComponent->MoveSlotQuantityToSlot(SourceSlotIndex, TargetSlotIndex, Quantity);
		break;
	case EPDItemContainerType::QuickSlot:
		// Quick slots are shortcuts to inventory items, not storage items.
		// Dropping a quick slot into stash is intentionally ignored.
		break;
	default:
		break;
	}
}

bool UPDStashWidget::ShouldOpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32& OutMaxQuantity, FText& OutTitle) const
{
	OutMaxQuantity = 0;
	OutTitle = FText::GetEmpty();

	if (SourceContainerType == EPDItemContainerType::Stash)
	{
		return false;
	}

	const FPDInventorySlot* SourceSlot = FindSourceSlot(SourceContainerType, SourceSlotIndex);
	const FPDInventorySlot* TargetSlot = FindStashSlot(TargetSlotIndex);
	if (!SourceSlot || !TargetSlot || SourceSlot->IsEmpty())
	{
		return false;
	}

	OutMaxQuantity = FPDItemSlotTransfer::GetQuantityLimit(*SourceSlot, *TargetSlot);
	OutTitle = SourceSlot->ItemData.DisplayName.IsEmpty() ? FText::FromName(SourceSlot->ItemData.ItemID) : SourceSlot->ItemData.DisplayName;
	return FPDItemSlotTransfer::CanPromptForPartial(*SourceSlot, *TargetSlot) && OutMaxQuantity > 1;
}

void UPDStashWidget::OpenQuantityPopup(int32 SlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		TakeStashSlotQuantity(SlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		TakeStashSlotQuantity(SlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = false;
	PendingSlotIndex = SlotIndex;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDStashWidget::OpenTransferQuantityPopup(EPDItemContainerType SourceContainerType, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 MaxQuantity, const FText& Title)
{
	if (!QuantityPopupWidgetClass)
	{
		ExecuteStashSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	if (ActiveQuantityPopup && ActiveQuantityPopup->IsInViewport())
	{
		ActiveQuantityPopup->RemoveFromParent();
	}

	ActiveQuantityPopup = CreateWidget<UPDQuantityPopupWidget>(GetOwningPlayer(), QuantityPopupWidgetClass);
	if (!ActiveQuantityPopup)
	{
		ExecuteStashSlotTransfer(SourceContainerType, SourceSlotIndex, TargetSlotIndex, MaxQuantity);
		return;
	}

	bPendingTransferQuantityRequest = true;
	PendingTransferSourceContainerType = SourceContainerType;
	PendingTransferSourceSlotIndex = SourceSlotIndex;
	PendingTransferTargetSlotIndex = TargetSlotIndex;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup->OnConfirmed.RemoveDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnConfirmed.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityConfirmed);
	ActiveQuantityPopup->OnCancelled.RemoveDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->OnCancelled.AddUniqueDynamic(this, &UPDStashWidget::HandleQuantityCancelled);
	ActiveQuantityPopup->AddToViewport(100);
	ActiveQuantityPopup->InitializeQuantityPopup(MaxQuantity, Title);
}

void UPDStashWidget::HandleQuantityConfirmed(int32 Quantity)
{
	if (bPendingTransferQuantityRequest)
	{
		ExecuteStashSlotTransfer(PendingTransferSourceContainerType, PendingTransferSourceSlotIndex, PendingTransferTargetSlotIndex, Quantity);
	}
	else if (PendingSlotIndex != INDEX_NONE)
	{
		TakeStashSlotQuantity(PendingSlotIndex, Quantity);
	}

	ClearQuantityRequest();
}

void UPDStashWidget::HandleQuantityCancelled()
{
	ClearQuantityRequest();
}

void UPDStashWidget::ClearQuantityRequest()
{
	bPendingTransferQuantityRequest = false;
	PendingTransferSourceContainerType = EPDItemContainerType::None;
	PendingTransferSourceSlotIndex = INDEX_NONE;
	PendingTransferTargetSlotIndex = INDEX_NONE;
	PendingSlotIndex = INDEX_NONE;
	ActiveQuantityPopup = nullptr;
}

void UPDStashWidget::BindStashChanged()
{
	UPDStashComponent* StashComponent = FindStashComponent();

	if (BoundStashComponent == StashComponent)
	{
		return;
	}

	UnbindStashChanged();

	BoundStashComponent = StashComponent;

	if (BoundStashComponent)
	{
		BoundStashComponent->OnStashChanged.AddUniqueDynamic(this, &UPDStashWidget::RefreshStashGrid);
	}
}

void UPDStashWidget::UnbindStashChanged()
{
	if (BoundStashComponent)
	{
		BoundStashComponent->OnStashChanged.RemoveDynamic(this, &UPDStashWidget::RefreshStashGrid);
		BoundStashComponent = nullptr;
	}
}
