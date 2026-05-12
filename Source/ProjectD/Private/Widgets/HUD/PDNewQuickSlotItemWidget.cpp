#include "Widgets/HUD/PDNewQuickSlotItemWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Core/PDPlayerController.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Items/PDStashComponent.h"

void UPDNewQuickSlotItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidget();
	if (WidgetTree)
	{
		if (UWidget* HotkeyWidget = WidgetTree->FindWidget(TEXT("Text_Hotkey")))
		{
			HotkeyWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	RefreshVisuals();
}

void UPDNewQuickSlotItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackWidget();
	if (WidgetTree)
	{
		if (UWidget* HotkeyWidget = WidgetTree->FindWidget(TEXT("Text_Hotkey")))
		{
			HotkeyWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	RefreshVisuals();
}

void UPDNewQuickSlotItemWidget::InitializeQuickSlot(UPDQuickSlotComponent* InQuickSlotComponent, int32 InSlotIndex)
{
	QuickSlotComponent = InQuickSlotComponent;
	SlotIndex = InSlotIndex;
}

void UPDNewQuickSlotItemWidget::SetSlotData(const FPDInventorySlot& InSlotData)
{
	SlotData = InSlotData;
	RefreshVisuals();
}

void UPDNewQuickSlotItemWidget::ClearSlotData()
{
	SlotData.Clear();
	RefreshVisuals();
}

FReply UPDNewQuickSlotItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !SlotData.IsEmpty())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPDNewQuickSlotItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (SlotData.IsEmpty() || SlotIndex == INDEX_NONE)
	{
		return;
	}

	UPDInventoryDragDropOperation* DragOperation = NewObject<UPDInventoryDragDropOperation>(this);
	if (!DragOperation)
	{
		return;
	}

	DragOperation->SourceContainerType = EPDItemContainerType::QuickSlot;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SlotData = SlotData;
	DragOperation->Pivot = EDragPivot::MouseDown;
	OutOperation = DragOperation;
}

bool UPDNewQuickSlotItemWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UPDInventoryDragDropOperation* DragOperation = Cast<UPDInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->IsValidPayload() || !QuickSlotComponent || SlotIndex == INDEX_NONE)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	const int32 Quantity = FMath::Max(1, DragOperation->SlotData.Quantity);
	bool bMoved = false;

	switch (DragOperation->SourceContainerType)
	{
	case EPDItemContainerType::Inventory:
		bMoved = QuickSlotComponent->StoreInventorySlotQuantityToSlot(FindInventoryComponent(), DragOperation->SourceSlotIndex, SlotIndex, Quantity);
		break;
	case EPDItemContainerType::Stash:
		bMoved = QuickSlotComponent->StoreStashSlotQuantityToSlot(FindStashComponent(), DragOperation->SourceSlotIndex, SlotIndex, Quantity);
		break;
	case EPDItemContainerType::QuickSlot:
		bMoved = QuickSlotComponent->MoveSlotQuantityToSlot(DragOperation->SourceSlotIndex, SlotIndex, Quantity);
		break;
	default:
		break;
	}

	return bMoved || Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UPDNewQuickSlotItemWidget::BuildFallbackWidget()
{
	if (!WidgetTree || GetRootWidget())
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(SlotSize.X);
	RootSizeBox->SetHeightOverride(SlotSize.Y);
	WidgetTree->RootWidget = RootSizeBox;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	RootSizeBox->AddChild(Overlay);

	SlotBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBackground"));
	SlotBackground->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.7f));
	if (UOverlaySlot* BorderSlot = Overlay->AddChildToOverlay(SlotBackground))
	{
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Image_ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_ItemIcon"));
	Image_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* ImageSlot = Overlay->AddChildToOverlay(Image_ItemIcon))
	{
		ImageSlot->SetPadding(FMargin(6.f));
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
	}


	Text_Quantity = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Quantity"));
	Text_Quantity->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text_Quantity->SetShadowOffset(FVector2D(1.f, 1.f));
	Text_Quantity->SetShadowColorAndOpacity(FLinearColor::Black);
	Text_Quantity->SetJustification(ETextJustify::Right);
	if (UOverlaySlot* QuantitySlot = Overlay->AddChildToOverlay(Text_Quantity))
	{
		QuantitySlot->SetPadding(FMargin(0.f, 0.f, 6.f, 4.f));
		QuantitySlot->SetHorizontalAlignment(HAlign_Right);
		QuantitySlot->SetVerticalAlignment(VAlign_Bottom);
	}
}

void UPDNewQuickSlotItemWidget::RefreshVisuals()
{
	BuildFallbackWidget();

	if (SlotData.IsEmpty())
	{
		if (Image_ItemIcon)
		{
			Image_ItemIcon->SetBrushFromTexture(nullptr);
			Image_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}

		if (Text_Quantity)
		{
			Text_Quantity->SetText(FText::GetEmpty());
		}
		return;
	}

	if (Image_ItemIcon)
	{
		Image_ItemIcon->SetBrushFromTexture(SlotData.ItemData.Icon);
		Image_ItemIcon->SetVisibility(SlotData.ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (Text_Quantity)
	{
		Text_Quantity->SetText(SlotData.Quantity > 1 ? FText::AsNumber(SlotData.Quantity) : FText::GetEmpty());
	}
}

UPDInventoryComponent* UPDNewQuickSlotItemWidget::FindInventoryComponent() const
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UPDInventoryComponent>();
	}
	return nullptr;
}

UPDStashComponent* UPDNewQuickSlotItemWidget::FindStashComponent() const
{
	// stash 인터페이스가 열려있을 때만 유효. PC가 ActiveStashComponent를 캐시.
	if (APDPlayerController* PC = Cast<APDPlayerController>(GetOwningPlayer()))
	{
		return PC->GetActiveStashComponent();
	}
	return nullptr;
}
