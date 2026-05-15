#include "Widgets/Inventory/PDMarketItemWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/PDInventoryComponent.h"

void UPDMarketItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveWidgets();
}

void UPDMarketItemWidget::SetMarketEntry(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex)
{
	MarketComponent = InMarketComponent;
	BuyerInventory = InBuyerInventory;
	Entry = InEntry;
	EntryIndex = InEntryIndex;
	ResolveWidgets();
	RefreshVisuals();
}

void UPDMarketItemWidget::ResolveWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!ImageItemIconWidget && !ImageItemIconWidgetName.IsNone())
	{
		ImageItemIconWidget = Cast<UImage>(WidgetTree->FindWidget(ImageItemIconWidgetName));
	}

	if (!TextItemNameWidget && !TextItemNameWidgetName.IsNone())
	{
		TextItemNameWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextItemNameWidgetName));
	}

	if (!TextPriceWidget && !TextPriceWidgetName.IsNone())
	{
		TextPriceWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextPriceWidgetName));
	}

	if (!TextStockWidget && !TextStockWidgetName.IsNone())
	{
		TextStockWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextStockWidgetName));
	}

	if (!ButtonBuyWidget && !ButtonBuyWidgetName.IsNone())
	{
		ButtonBuyWidget = Cast<UButton>(WidgetTree->FindWidget(ButtonBuyWidgetName));
		if (ButtonBuyWidget)
		{
			ButtonBuyWidget->OnClicked.RemoveDynamic(this, &UPDMarketItemWidget::HandleBuyClicked);
			ButtonBuyWidget->OnClicked.AddUniqueDynamic(this, &UPDMarketItemWidget::HandleBuyClicked);
		}
	}
}

void UPDMarketItemWidget::RefreshVisuals()
{
	FPDItemData ItemData;
	if (MarketComponent)
	{
		MarketComponent->ResolveEntryItemData(Entry, ItemData);
	}

	const bool bLocked = IsLocked();
	const int32 RequiredLevel = GetRequiredTraderLevel();
	const FText LockedToolTip = FText::FromString(FString::Printf(TEXT("상인 평판 레벨 %d 필요"), RequiredLevel));

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetBrushFromTexture(bLocked ? nullptr : ItemData.Icon);
		ImageItemIconWidget->SetVisibility(!bLocked && ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		ImageItemIconWidget->SetToolTipText(bLocked ? LockedToolTip : FText::GetEmpty());
	}

	if (TextItemNameWidget)
	{
		const FText DisplayName = ItemData.DisplayName.IsEmpty() ? FText::FromName(ItemData.ItemID) : ItemData.DisplayName;
		TextItemNameWidget->SetText(bLocked ? FText::FromString(LockedItemNameString) : DisplayName);
		TextItemNameWidget->SetToolTipText(bLocked ? LockedToolTip : FText::GetEmpty());
	}

	if (TextPriceWidget)
	{
		TextPriceWidget->SetText(bLocked ? FText::FromString(TEXT("-")) : FText::AsNumber(GetUnitPrice()));
		TextPriceWidget->SetToolTipText(bLocked ? LockedToolTip : FText::GetEmpty());
	}

	if (TextStockWidget)
	{
		TextStockWidget->SetText(bLocked ? FText::FromString(TEXT("-")) : (Entry.Stock < 0 ? FText::FromString(TEXT("∞")) : FText::AsNumber(Entry.Stock)));
		TextStockWidget->SetToolTipText(bLocked ? LockedToolTip : FText::GetEmpty());
	}

	if (ButtonBuyWidget)
	{
		ButtonBuyWidget->SetIsEnabled(!bLocked);
		ButtonBuyWidget->SetToolTipText(bLocked ? LockedToolTip : FText::GetEmpty());
	}

	SetToolTipText(bLocked ? LockedToolTip : FText::GetEmpty());
}

int32 UPDMarketItemWidget::GetUnitPrice() const
{
	return MarketComponent ? MarketComponent->GetEntryUnitPrice(Entry) : 0;
}

bool UPDMarketItemWidget::IsLocked() const
{
	return MarketComponent && !MarketComponent->CanBuyEntry(EntryIndex);
}

int32 UPDMarketItemWidget::GetRequiredTraderLevel() const
{
	return MarketComponent ? MarketComponent->GetRequiredTraderLevelForEntry(EntryIndex) : 1;
}

void UPDMarketItemWidget::HandleBuyClicked()
{
	if (!MarketComponent || !BuyerInventory || EntryIndex == INDEX_NONE)
	{
		return;
	}

	MarketComponent->BuyEntry(BuyerInventory, EntryIndex, 1);
}
