#include "Widgets/Inventory/PDInventorySlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "Materials/MaterialInterface.h"

void UPDInventorySlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveTextWidgets();
	ResolveOverlayWidgets();
	RefreshVisuals();
}

FReply UPDInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bLastClickWithControl = InMouseEvent.IsControlDown();
		OnSlotLeftClicked.Broadcast(this, SlotIndex);

		if (bLastClickWithControl || SlotData.IsEmpty() || SlotContainerType == EPDItemContainerType::None)
		{
			return FReply::Handled();
		}

		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bLastClickWithControl = InMouseEvent.IsControlDown();
		OnSlotRightClicked.Broadcast(this, SlotIndex);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPDInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!SlotData.IsEmpty())
	{
		OnSlotHovered.Broadcast(this, SlotIndex);
	}

	if (ImageHoverBorderWidget)
	{
		ImageHoverBorderWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPDInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	OnSlotUnhovered.Broadcast(this, SlotIndex);

	if (ImageHoverBorderWidget)
	{
		ImageHoverBorderWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPDInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (SlotData.IsEmpty() || SlotContainerType == EPDItemContainerType::None || SlotIndex == INDEX_NONE)
	{
		return;
	}

	UPDInventoryDragDropOperation* DragOperation = NewObject<UPDInventoryDragDropOperation>(this);
	if (!DragOperation)
	{
		return;
	}

	DragOperation->SourceContainerType = SlotContainerType;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SlotData = SlotData;
	DragOperation->DefaultDragVisual = CreateDragVisualWidget();
	DragOperation->Pivot = EDragPivot::MouseDown;

	OutOperation = DragOperation;
}

bool UPDInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ClearDropOverlays();

	if (UPDInventoryDragDropOperation* DragOperation = Cast<UPDInventoryDragDropOperation>(InOperation))
	{
		if (DragOperation->IsValidPayload() && SlotIndex != INDEX_NONE && OnSlotItemDropped.IsBound())
		{
			OnSlotItemDropped.Broadcast(this, SlotIndex, DragOperation);
			return true;
		}
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

/* --- 추가 Start--- */
bool UPDInventorySlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const bool bSuperHandled = Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);

	const bool bCanAccept = CanAcceptDrop(InOperation);

	if (ImageDropValidWidget)
	{
		ImageDropValidWidget->SetVisibility(bCanAccept ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ImageDropInvalidWidget)
	{
		ImageDropInvalidWidget->SetVisibility(bCanAccept ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	return bSuperHandled || bCanAccept;
}

void UPDInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	ClearDropOverlays();
}

bool UPDInventorySlotWidget::CanAcceptDrop(UDragDropOperation* InOperation) const
{
	UPDInventoryDragDropOperation* DragOperation = Cast<UPDInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->IsValidPayload())
	{
		return false;
	}

	if (DragOperation->SourceContainerType == SlotContainerType && DragOperation->SourceSlotIndex == SlotIndex)
	{
		return false;
	}

	return true;
}
/* --- 추가 End--- */

void UPDInventorySlotWidget::SetSlotData(const FPDInventorySlot& InSlotData, int32 InSlotIndex)
{
	SlotData = InSlotData;
	SlotIndex = InSlotIndex;
	ResolveTextWidgets();
	ResolveOverlayWidgets();
	RefreshVisuals();
}

void UPDInventorySlotWidget::ClearSlotData(int32 InSlotIndex)
{
	SlotData.Clear();
	SlotIndex = InSlotIndex;
	ResolveTextWidgets();
	ResolveOverlayWidgets();
	RefreshVisuals();
}

void UPDInventorySlotWidget::SetSlotContainerType(EPDItemContainerType InSlotContainerType)
{
	SlotContainerType = InSlotContainerType;
}

void UPDInventorySlotWidget::ResolveTextWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!TextItemNameWidget && !TextItemNameWidgetName.IsNone())
	{
		TextItemNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemNameWidgetName));
	}

	if (!TextQuantityWidget && !TextQuantityWidgetName.IsNone())
	{
		TextQuantityWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextQuantityWidgetName));
	}

	if (!ImageItemIconWidget && !ImageItemIconWidgetName.IsNone())
	{
		ImageItemIconWidget = Cast<UImage>(WidgetTree->FindWidget(ImageItemIconWidgetName));
	}
}

/* --- 추가 Start--- */
void UPDInventorySlotWidget::ResolveOverlayWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!ImageSlotBGWidget && !ImageSlotBGWidgetName.IsNone())
	{
		ImageSlotBGWidget = Cast<UImage>(WidgetTree->FindWidget(ImageSlotBGWidgetName));
	}

	if (!ImageHoverBorderWidget && !ImageHoverBorderWidgetName.IsNone())
	{
		ImageHoverBorderWidget = Cast<UImage>(WidgetTree->FindWidget(ImageHoverBorderWidgetName));
	}

	if (!ImageDropValidWidget && !ImageDropValidWidgetName.IsNone())
	{
		ImageDropValidWidget = Cast<UImage>(WidgetTree->FindWidget(ImageDropValidWidgetName));
	}

	if (!ImageDropInvalidWidget && !ImageDropInvalidWidgetName.IsNone())
	{
		ImageDropInvalidWidget = Cast<UImage>(WidgetTree->FindWidget(ImageDropInvalidWidgetName));
	}
}

void UPDInventorySlotWidget::ClearDropOverlays()
{
	if (ImageDropValidWidget)
	{
		ImageDropValidWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ImageDropInvalidWidget)
	{
		ImageDropInvalidWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
/* --- 추가 End--- */

void UPDInventorySlotWidget::RefreshVisuals()
{
	ResolveTextWidgets();
	ResolveOverlayWidgets();

	if (ImageSlotBGWidget)
	{
		UMaterialInterface* TargetMaterial = SlotData.IsEmpty()
			? SlotBGMaterial_Empty.LoadSynchronous()
			: SlotBGMaterial_Filled.LoadSynchronous();

		if (TargetMaterial)
		{
			ImageSlotBGWidget->SetBrushFromMaterial(TargetMaterial);
		}
	}

	if (SlotData.IsEmpty())
	{
		CachedTooltipDisplayName = FText::GetEmpty();
		CachedTooltipDescription = FText::GetEmpty();
		ClearTooltip();

		if (TextItemNameWidget)
		{
			TextItemNameWidget->SetText(FText::GetEmpty());
			TextItemNameWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (TextQuantityWidget)
		{
			TextQuantityWidget->SetText(FText::GetEmpty());
		}

		if (ImageItemIconWidget)
		{
			ImageItemIconWidget->SetBrushFromTexture(nullptr);
			ImageItemIconWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		return;
	}

	const FText DisplayName = SlotData.ItemData.DisplayName.IsEmpty() ? FText::FromName(SlotData.ItemData.ItemID) : SlotData.ItemData.DisplayName;
	const FText Description = SlotData.ItemData.Description;
	CachedTooltipDisplayName = DisplayName;
	CachedTooltipDescription = Description;

	if (TextItemNameWidget)
	{
		TextItemNameWidget->SetText(DisplayName);
		TextItemNameWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (TextQuantityWidget)
	{
		TextQuantityWidget->SetText(SlotData.Quantity > 1 ? FText::AsNumber(SlotData.Quantity) : FText::GetEmpty());
	}

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetBrushFromTexture(SlotData.ItemData.Icon);
		ImageItemIconWidget->SetVisibility(SlotData.ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	ApplyTooltip(DisplayName, Description);
}

void UPDInventorySlotWidget::ApplyTooltip(const FText& DisplayName, const FText& Description)
{
	CachedTooltipDisplayName = DisplayName;
	CachedTooltipDescription = Description;
	ClearTooltip();
}

UUserWidget* UPDInventorySlotWidget::CreateItemTooltipWidget()
{
	if (SlotData.IsEmpty() || !TooltipWidgetClass)
	{
		return nullptr;
	}

	UUserWidget* TooltipWidget = GetOwningPlayer()
		? CreateWidget<UUserWidget>(GetOwningPlayer(), TooltipWidgetClass)
		: CreateWidget<UUserWidget>(GetWorld(), TooltipWidgetClass);

	ApplyTooltipTextToWidget(TooltipWidget, CachedTooltipDisplayName, CachedTooltipDescription);
	return TooltipWidget;
}

void UPDInventorySlotWidget::ApplyTooltipTextToWidget(UUserWidget* TooltipWidget, const FText& DisplayName, const FText& Description) const
{
	if (!TooltipWidget || !TooltipWidget->WidgetTree)
	{
		return;
	}

	if (UTextBlock* TooltipNameText = Cast<UTextBlock>(TooltipWidget->WidgetTree->FindWidget(TooltipItemNameWidgetName)))
	{
		TooltipNameText->SetText(DisplayName);
	}

	if (UTextBlock* TooltipDescriptionText = Cast<UTextBlock>(TooltipWidget->WidgetTree->FindWidget(TooltipDescriptionWidgetName)))
	{
		TooltipDescriptionText->SetText(Description);
	}

	if (UTextBlock* TooltipWeightText = Cast<UTextBlock>(TooltipWidget->WidgetTree->FindWidget(TooltipWeightWidgetName)))
	{
		TooltipWeightText->SetText(FText::FromString(FString::Printf(TEXT("Weight: %.1f"), FMath::Max(0.f, SlotData.ItemData.Weight))));
	}
}

void UPDInventorySlotWidget::ClearTooltip()
{
	SetToolTip(nullptr);
	SetToolTipText(FText::GetEmpty());
}

UPDInventorySlotWidget* UPDInventorySlotWidget::CreateDragVisualWidget() const
{
	UPDInventorySlotWidget* DragVisual = GetOwningPlayer()
		? CreateWidget<UPDInventorySlotWidget>(GetOwningPlayer(), GetClass())
		: CreateWidget<UPDInventorySlotWidget>(GetWorld(), GetClass());

	if (!DragVisual)
	{
		return nullptr;
	}

	DragVisual->SetSlotContainerType(EPDItemContainerType::None);
	DragVisual->SetSlotData(SlotData, SlotIndex);
	DragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
	DragVisual->SetRenderOpacity(0.85f);
	return DragVisual;
}
