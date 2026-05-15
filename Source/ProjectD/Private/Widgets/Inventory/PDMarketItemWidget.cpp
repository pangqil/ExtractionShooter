#include "Widgets/Inventory/PDMarketItemWidget.h"

#include "Blueprint/UserWidget.h"
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


void UPDMarketItemWidget::SetShowLockedRequirementText(bool bInShow)
{
	bShowLockedRequirementText = bInShow;
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

	if (!TextRequiredTraderLevelWidget && !TextRequiredTraderLevelWidgetName.IsNone())
	{
		TextRequiredTraderLevelWidget = Cast<UTextBlock>(WidgetTree->FindWidget(TextRequiredTraderLevelWidgetName));
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
	const FText LockedRequirementText = FText::FromString(FString::Printf(TEXT("마켓 레벨 %d 필요"), RequiredLevel));

	ClearMarketTooltips();

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetBrushFromTexture(bLocked ? nullptr : ItemData.Icon);
		ImageItemIconWidget->SetVisibility(!bLocked && ItemData.Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (TextItemNameWidget)
	{
		const FText DisplayName = ItemData.DisplayName.IsEmpty() ? FText::FromName(ItemData.ItemID) : ItemData.DisplayName;
		TextItemNameWidget->SetText(bLocked ? FText::FromString(LockedItemNameString) : DisplayName);
	}

	if (TextPriceWidget)
	{
		TextPriceWidget->SetText(bLocked ? FText::FromString(TEXT("-")) : FText::AsNumber(GetUnitPrice()));
	}

	if (TextStockWidget)
	{
		TextStockWidget->SetText(bLocked ? FText::FromString(TEXT("-")) : (Entry.Stock < 0 ? FText::FromString(TEXT("∞")) : FText::AsNumber(Entry.Stock)));
	}

	if (ButtonBuyWidget)
	{
		ButtonBuyWidget->SetIsEnabled(!bLocked);
	}

	if (TextRequiredTraderLevelWidget)
	{
		const bool bShowRequirementText = bLocked && bShowLockedRequirementText;
		TextRequiredTraderLevelWidget->SetText(LockedRequirementText);
		TextRequiredTraderLevelWidget->SetVisibility(bShowRequirementText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (bLocked)
	{
		SetToolTip(CreateLockedRequirementTooltipWidget(LockedRequirementText));
	}
}

void UPDMarketItemWidget::ClearMarketTooltips()
{
	SetToolTip(nullptr);
	SetToolTipText(FText::GetEmpty());

	if (ImageItemIconWidget)
	{
		ImageItemIconWidget->SetToolTip(nullptr);
		ImageItemIconWidget->SetToolTipText(FText::GetEmpty());
	}

	if (TextItemNameWidget)
	{
		TextItemNameWidget->SetToolTip(nullptr);
		TextItemNameWidget->SetToolTipText(FText::GetEmpty());
	}

	if (TextPriceWidget)
	{
		TextPriceWidget->SetToolTip(nullptr);
		TextPriceWidget->SetToolTipText(FText::GetEmpty());
	}

	if (TextStockWidget)
	{
		TextStockWidget->SetToolTip(nullptr);
		TextStockWidget->SetToolTipText(FText::GetEmpty());
	}

	if (ButtonBuyWidget)
	{
		ButtonBuyWidget->SetToolTip(nullptr);
		ButtonBuyWidget->SetToolTipText(FText::GetEmpty());
	}
}

UUserWidget* UPDMarketItemWidget::CreateLockedRequirementTooltipWidget(const FText& RequirementText) const
{
	if (!LockedTooltipWidgetClass)
	{
		return nullptr;
	}

	UUserWidget* TooltipWidget = GetOwningPlayer()
		? CreateWidget<UUserWidget>(GetOwningPlayer(), LockedTooltipWidgetClass)
		: CreateWidget<UUserWidget>(GetWorld(), LockedTooltipWidgetClass);

	if (!TooltipWidget || !TooltipWidget->WidgetTree)
	{
		return TooltipWidget;
	}

	if (UTextBlock* RequiredLevelText = Cast<UTextBlock>(TooltipWidget->WidgetTree->FindWidget(LockedTooltipRequiredLevelTextWidgetName)))
	{
		RequiredLevelText->SetText(RequirementText);
	}

	return TooltipWidget;
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
