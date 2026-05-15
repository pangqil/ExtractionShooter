#include "Widgets/Inventory/PDMarketWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "GameFramework/Pawn.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDMarketComponent.h"
#include "Widgets/Inventory/PDMarketItemWidget.h"

void UPDMarketWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindMarketChanged();
	RefreshMarketGoods();
}

void UPDMarketWidget::NativeDestruct()
{
	UnbindMarketChanged();
	Super::NativeDestruct();
}

void UPDMarketWidget::InitializeMarket(UPDMarketComponent* InMarketComponent)
{
	MarketComponent = InMarketComponent;
	if (MarketComponent)
	{
		MarketComponent->SyncTraderReputationFromSave();
	}
	BindMarketChanged();
	RefreshMarketGoods();
}

void UPDMarketWidget::RefreshMarketGoods()
{
	ResolveMarketGridPanel();

	if (!MarketGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("PDMarketWidget: Market grid widget was not found. Expected widget name: %s"), *MarketGridWidgetName.ToString());
		return;
	}

	MarketGridPanel->ClearChildren();

	if (!MarketComponent || !MarketItemWidgetClass)
	{
		return;
	}

	UPDInventoryComponent* BuyerInventory = FindInventoryComponent();
	const int32 Columns = FMath::Max(1, MarketGridColumns);

	int32 VisibleIndex = 0;
	for (int32 Index = 0; Index < MarketComponent->Goods.Num(); ++Index)
	{
		if (!MarketComponent->ShouldShowEntry(Index))
		{
			continue;
		}

		UUserWidget* CreatedWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), MarketItemWidgetClass);
		if (!CreatedWidget)
		{
			continue;
		}

		if (UPDMarketItemWidget* MarketItemWidget = Cast<UPDMarketItemWidget>(CreatedWidget))
		{
			MarketItemWidget->SetMarketEntry(MarketComponent, BuyerInventory, MarketComponent->Goods[Index], Index);
		}

		UUniformGridSlot* GridSlot = MarketGridPanel->AddChildToUniformGrid(CreatedWidget, VisibleIndex / Columns, VisibleIndex % Columns);
		++VisibleIndex;
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UPDMarketWidget::ResolveMarketGridPanel()
{
	if (MarketGridPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* FoundWidget = WidgetTree->FindWidget(MarketGridWidgetName))
	{
		MarketGridPanel = Cast<UUniformGridPanel>(FoundWidget);
	}

	if (!MarketGridPanel)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!MarketGridPanel)
			{
				MarketGridPanel = Cast<UUniformGridPanel>(Widget);
			}
		});
	}
}

UPDInventoryComponent* UPDMarketWidget::FindInventoryComponent() const
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn->FindComponentByClass<UPDInventoryComponent>();
	}

	return nullptr;
}

void UPDMarketWidget::BindMarketChanged()
{
	if (BoundMarketComponent == MarketComponent)
	{
		return;
	}

	UnbindMarketChanged();

	BoundMarketComponent = MarketComponent;

	if (BoundMarketComponent)
	{
		BoundMarketComponent->OnMarketChanged.AddUniqueDynamic(this, &UPDMarketWidget::RefreshMarketGoods);
	}
}

void UPDMarketWidget::UnbindMarketChanged()
{
	if (BoundMarketComponent)
	{
		BoundMarketComponent->OnMarketChanged.RemoveDynamic(this, &UPDMarketWidget::RefreshMarketGoods);
		BoundMarketComponent = nullptr;
	}
}
