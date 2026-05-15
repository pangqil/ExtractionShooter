#include "Widgets/Inventory/PDEquipmentModificationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/Pawn.h"
#include "Items/PDEquipmentModificationComponent.h"
#include "Items/PDInventoryComponent.h"
#include "Widgets/Inventory/PDInventorySlotWidget.h"

void UPDEquipmentModificationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveComponents();
	ResolveWidgets();
	BindWidgetEvents();
	BindComponentEvents();
	RefreshAll();
}

void UPDEquipmentModificationWidget::NativeDestruct()
{
	UnbindWidgetEvents();
	UnbindComponentEvents();
	Super::NativeDestruct();
}

void UPDEquipmentModificationWidget::ResolveComponents()
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	InventoryComponent = OwningPawn ? OwningPawn->FindComponentByClass<UPDInventoryComponent>() : nullptr;
	ModificationComponent = OwningPawn ? OwningPawn->FindComponentByClass<UPDEquipmentModificationComponent>() : nullptr;
}

void UPDEquipmentModificationWidget::ResolveWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	ScrollBoxEquipmentList = Cast<UScrollBox>(WidgetTree->FindWidget(ScrollBoxEquipmentListWidgetName));
	ImageSelectedItemIcon = Cast<UImage>(WidgetTree->FindWidget(ImageSelectedItemIconWidgetName));
	TextSelectedItemName = Cast<UTextBlock>(WidgetTree->FindWidget(TextSelectedItemNameWidgetName));
	TextCurrentLevel = Cast<UTextBlock>(WidgetTree->FindWidget(TextCurrentLevelWidgetName));
	TextTargetLevel = Cast<UTextBlock>(WidgetTree->FindWidget(TextTargetLevelWidgetName));
	TextStatType = Cast<UTextBlock>(WidgetTree->FindWidget(TextStatTypeWidgetName));
	TextStatPreview = Cast<UTextBlock>(WidgetTree->FindWidget(TextStatPreviewWidgetName));
	TextGoldCost = Cast<UTextBlock>(WidgetTree->FindWidget(TextGoldCostWidgetName));
	VerticalBoxRequiredMaterials = Cast<UVerticalBox>(WidgetTree->FindWidget(VerticalBoxRequiredMaterialsWidgetName));
	TextBaseSuccessRate = Cast<UTextBlock>(WidgetTree->FindWidget(TextBaseSuccessRateWidgetName));
	TextBoostSuccessRate = Cast<UTextBlock>(WidgetTree->FindWidget(TextBoostSuccessRateWidgetName));
	TextFinalSuccessRate = Cast<UTextBlock>(WidgetTree->FindWidget(TextFinalSuccessRateWidgetName));
	ButtonBoostNone = Cast<UButton>(WidgetTree->FindWidget(ButtonBoostNoneWidgetName));
	ButtonBoostLow = Cast<UButton>(WidgetTree->FindWidget(ButtonBoostLowWidgetName));
	ButtonBoostMid = Cast<UButton>(WidgetTree->FindWidget(ButtonBoostMidWidgetName));
	ButtonBoostHigh = Cast<UButton>(WidgetTree->FindWidget(ButtonBoostHighWidgetName));
	ButtonModify = Cast<UButton>(WidgetTree->FindWidget(ButtonModifyWidgetName));
	TextResult = Cast<UTextBlock>(WidgetTree->FindWidget(TextResultWidgetName));
}

void UPDEquipmentModificationWidget::BindWidgetEvents()
{
	if (ButtonBoostNone)
	{
		ButtonBoostNone->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostNoneClicked);
	}
	if (ButtonBoostLow)
	{
		ButtonBoostLow->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostLowClicked);
	}
	if (ButtonBoostMid)
	{
		ButtonBoostMid->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostMidClicked);
	}
	if (ButtonBoostHigh)
	{
		ButtonBoostHigh->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleBoostHighClicked);
	}
	if (ButtonModify)
	{
		ButtonModify->OnClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleModifyClicked);
	}
}

void UPDEquipmentModificationWidget::UnbindWidgetEvents()
{
	if (ButtonBoostNone)
	{
		ButtonBoostNone->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostNoneClicked);
	}
	if (ButtonBoostLow)
	{
		ButtonBoostLow->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostLowClicked);
	}
	if (ButtonBoostMid)
	{
		ButtonBoostMid->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostMidClicked);
	}
	if (ButtonBoostHigh)
	{
		ButtonBoostHigh->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleBoostHighClicked);
	}
	if (ButtonModify)
	{
		ButtonModify->OnClicked.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleModifyClicked);
	}
}

void UPDEquipmentModificationWidget::BindComponentEvents()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleInventoryChanged);
	}
	if (ModificationComponent)
	{
		ModificationComponent->OnModificationFinished.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleModificationFinished);
	}
}

void UPDEquipmentModificationWidget::UnbindComponentEvents()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleInventoryChanged);
	}
	if (ModificationComponent)
	{
		ModificationComponent->OnModificationFinished.RemoveDynamic(this, &UPDEquipmentModificationWidget::HandleModificationFinished);
	}
}

void UPDEquipmentModificationWidget::RefreshAll()
{
	RefreshEquipmentList();
	SelectFirstEquipmentSlotIfNeeded();
	RefreshPreview();
}

void UPDEquipmentModificationWidget::RefreshEquipmentList()
{
	if (!ScrollBoxEquipmentList)
	{
		return;
	}

	ScrollBoxEquipmentList->ClearChildren();

	if (!InventoryComponent || !EquipmentSlotWidgetClass)
	{
		return;
	}

	for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
	{
		const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[Index];
		if (InventorySlotData.IsEmpty() || InventorySlotData.ItemData.ItemType != EPDItemType::Equipment)
		{
			continue;
		}

		UPDInventorySlotWidget* SlotWidget = CreateWidget<UPDInventorySlotWidget>(GetOwningPlayer(), EquipmentSlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotContainerType(EPDItemContainerType::Inventory);
		SlotWidget->SetSlotData(InventorySlotData, Index);
		SlotWidget->OnSlotLeftClicked.AddUniqueDynamic(this, &UPDEquipmentModificationWidget::HandleEquipmentSlotClicked);
		ScrollBoxEquipmentList->AddChild(SlotWidget);
	}
}

void UPDEquipmentModificationWidget::SelectFirstEquipmentSlotIfNeeded()
{
	if (SelectedSlotIndex != INDEX_NONE || !InventoryComponent)
	{
		return;
	}

	for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
	{
		const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[Index];
		if (!InventorySlotData.IsEmpty() && InventorySlotData.ItemData.ItemType == EPDItemType::Equipment)
		{
			SelectedSlotIndex = Index;
			return;
		}
	}
}

void UPDEquipmentModificationWidget::SelectInventorySlot(int32 SlotIndex)
{
	SelectedSlotIndex = SlotIndex;
	SetText(TextResult, ModifyReadyText);
	RefreshPreview();
}

void UPDEquipmentModificationWidget::SetBoostType(EPDModificationBoostType BoostType)
{
	SelectedBoostType = BoostType;
	SetText(TextResult, ModifyReadyText);
	RefreshPreview();
}

void UPDEquipmentModificationWidget::RefreshPreview()
{
	bCurrentPreviewValid = false;
	CurrentPreview = FPDModificationPreview();
	CurrentPreviewResult = EPDModificationResult::InvalidSlot;

	if (!InventoryComponent || !ModificationComponent || !InventoryComponent->Items.IsValidIndex(SelectedSlotIndex))
	{
		ClearPreview();
		return;
	}

	const FPDInventorySlot& InventorySlotData = InventoryComponent->Items[SelectedSlotIndex];
	if (InventorySlotData.IsEmpty() || InventorySlotData.ItemData.ItemType != EPDItemType::Equipment)
	{
		ClearPreview();
		return;
	}

	bCurrentPreviewValid = ModificationComponent->CanModifyInventorySlotWithBoost(InventoryComponent, SelectedSlotIndex, SelectedBoostType, CurrentPreview, CurrentPreviewResult);
	if (!bCurrentPreviewValid && CurrentPreviewResult == EPDModificationResult::AlreadyMaxLevel)
	{
		ModificationComponent->GetModificationPreviewWithBoost(InventorySlotData, SelectedBoostType, CurrentPreview, CurrentPreviewResult);
	}

	ApplyPreview(CurrentPreview, InventorySlotData);
}

void UPDEquipmentModificationWidget::ClearPreview()
{
	SetText(TextSelectedItemName, EmptySelectionText);
	SetText(TextCurrentLevel, FText::FromString(TEXT("-")));
	SetText(TextTargetLevel, FText::FromString(TEXT("-")));
	SetText(TextStatType, FText::FromString(TEXT("-")));
	SetText(TextStatPreview, FText::FromString(TEXT("-")));
	SetText(TextGoldCost, FText::FromString(TEXT("-")));
	SetText(TextBaseSuccessRate, FText::FromString(TEXT("-")));
	SetText(TextBoostSuccessRate, FText::FromString(TEXT("-")));
	SetText(TextFinalSuccessRate, FText::FromString(TEXT("-")));
	if (ImageSelectedItemIcon)
	{
		ImageSelectedItemIcon->SetBrushFromTexture(nullptr);
	}
	if (VerticalBoxRequiredMaterials)
	{
		VerticalBoxRequiredMaterials->ClearChildren();
	}
	if (ButtonModify)
	{
		ButtonModify->SetIsEnabled(false);
	}
}

void UPDEquipmentModificationWidget::ApplyPreview(const FPDModificationPreview& Preview, const FPDInventorySlot& SlotData)
{
	const FPDItemData& ItemData = SlotData.ItemData;
	SetText(TextSelectedItemName, ItemData.DisplayName.IsEmpty() ? FText::FromName(ItemData.ItemID) : ItemData.DisplayName);
	SetText(TextCurrentLevel, FormatLevelText(Preview.CurrentModificationLevel));

	if (CurrentPreviewResult == EPDModificationResult::AlreadyMaxLevel)
	{
		SetText(TextTargetLevel, FText::FromString(TEXT("MAX")));
	}
	else
	{
		SetText(TextTargetLevel, FormatLevelText(Preview.TargetModificationLevel));
	}

	if (ImageSelectedItemIcon)
	{
		ImageSelectedItemIcon->SetBrushFromTexture(ItemData.Icon);
	}

	if (Preview.AttackBonus > 0.f)
	{
		SetText(TextStatType, FText::FromString(TEXT("공격력")));
		SetText(TextStatPreview, FText::FromString(FString::Printf(TEXT("+%.1f"), Preview.AttackBonus)));
	}
	else if (Preview.DefenseBonus > 0.f)
	{
		SetText(TextStatType, FText::FromString(TEXT("방어력")));
		SetText(TextStatPreview, FText::FromString(FString::Printf(TEXT("+%.1f"), Preview.DefenseBonus)));
	}
	else
	{
		SetText(TextStatType, FText::FromString(TEXT("-")));
		SetText(TextStatPreview, FText::FromString(TEXT("-")));
	}

	SetText(TextGoldCost, FText::FromString(FString::Printf(TEXT("%d / %d"), InventoryComponent ? InventoryComponent->GetGold() : 0, Preview.GoldCost)));
	SetText(TextBaseSuccessRate, FormatPercent(Preview.BaseSuccessRate));
	SetText(TextBoostSuccessRate, FText::FromString(FString::Printf(TEXT("+%s"), *FormatPercent(Preview.BoostSuccessRate).ToString())));
	SetText(TextFinalSuccessRate, FormatPercent(Preview.SuccessRate));
	RefreshMaterialList(Preview);

	if (ButtonModify)
	{
		ButtonModify->SetIsEnabled(bCurrentPreviewValid);
	}
}

void UPDEquipmentModificationWidget::RefreshMaterialList(const FPDModificationPreview& Preview)
{
	if (!VerticalBoxRequiredMaterials || !WidgetTree)
	{
		return;
	}

	VerticalBoxRequiredMaterials->ClearChildren();

	for (const FPDModificationMaterialRequirement& Material : Preview.RequiredMaterials)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!TextBlock)
		{
			continue;
		}

		TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s  %d / %d"), *Material.RequiredItemID.ToString(), CountItem(Material.RequiredItemID), Material.Quantity)));
		VerticalBoxRequiredMaterials->AddChild(TextBlock);
	}

	if (!Preview.BoostItemID.IsNone() && Preview.BoostItemQuantity > 0)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (TextBlock)
		{
			TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s  %d / %d"), *Preview.BoostItemID.ToString(), CountItem(Preview.BoostItemID), Preview.BoostItemQuantity)));
			VerticalBoxRequiredMaterials->AddChild(TextBlock);
		}
	}
}

void UPDEquipmentModificationWidget::HandleEquipmentSlotClicked(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex)
{
	SelectInventorySlot(SlotIndex);
}

void UPDEquipmentModificationWidget::HandleBoostNoneClicked()
{
	SetBoostType(EPDModificationBoostType::None);
}

void UPDEquipmentModificationWidget::HandleBoostLowClicked()
{
	SetBoostType(EPDModificationBoostType::Low);
}

void UPDEquipmentModificationWidget::HandleBoostMidClicked()
{
	SetBoostType(EPDModificationBoostType::Mid);
}

void UPDEquipmentModificationWidget::HandleBoostHighClicked()
{
	SetBoostType(EPDModificationBoostType::High);
}

void UPDEquipmentModificationWidget::HandleModifyClicked()
{
	if (!InventoryComponent || !ModificationComponent || SelectedSlotIndex == INDEX_NONE)
	{
		SetText(TextResult, GetResultText(EPDModificationResult::InvalidSlot, false));
		return;
	}

	FPDModificationPreview Preview;
	EPDModificationResult Result = EPDModificationResult::InvalidSlot;
	const bool bSuccess = ModificationComponent->TryModifyInventorySlotWithBoost(InventoryComponent, SelectedSlotIndex, SelectedBoostType, Preview, Result);
	SetText(TextResult, GetResultText(Result, bSuccess));
	RefreshEquipmentList();
	RefreshPreview();
}

void UPDEquipmentModificationWidget::HandleInventoryChanged()
{
	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(SelectedSlotIndex) || InventoryComponent->Items[SelectedSlotIndex].IsEmpty())
	{
		SelectedSlotIndex = INDEX_NONE;
	}
	RefreshAll();
}

void UPDEquipmentModificationWidget::HandleModificationFinished(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result)
{
	SetText(TextResult, GetResultText(Result, bSuccess));
}

void UPDEquipmentModificationWidget::SetText(UTextBlock* TextBlock, const FText& Text) const
{
	if (TextBlock)
	{
		TextBlock->SetText(Text);
	}
}

void UPDEquipmentModificationWidget::SetTextByName(FName WidgetName, const FText& Text) const
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return;
	}

	if (UTextBlock* TextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName)))
	{
		TextBlock->SetText(Text);
	}
}

int32 UPDEquipmentModificationWidget::CountItem(FName ItemID) const
{
	if (!InventoryComponent || ItemID.IsNone())
	{
		return 0;
	}

	int32 Count = 0;
	for (const FPDInventorySlot& InventorySlotData : InventoryComponent->Items)
	{
		if (!InventorySlotData.IsEmpty() && InventorySlotData.ItemData.ItemID == ItemID)
		{
			Count += InventorySlotData.Quantity;
		}
	}
	return Count;
}

FText UPDEquipmentModificationWidget::GetResultText(EPDModificationResult Result, bool bSuccess) const
{
	if (Result == EPDModificationResult::Success && bSuccess)
	{
		return FText::FromString(TEXT("개조 성공"));
	}

	switch (Result)
	{
	case EPDModificationResult::FailedByChance:
		return FText::FromString(TEXT("개조 실패"));
	case EPDModificationResult::InvalidInventory:
		return FText::FromString(TEXT("인벤토리 오류"));
	case EPDModificationResult::InvalidSlot:
		return FText::FromString(TEXT("장비를 선택하세요"));
	case EPDModificationResult::NotEquipment:
		return FText::FromString(TEXT("장비만 개조할 수 있습니다"));
	case EPDModificationResult::AlreadyMaxLevel:
		return FText::FromString(TEXT("최대 개조 단계입니다"));
	case EPDModificationResult::MissingCurveTable:
		return FText::FromString(TEXT("개조 데이터가 없습니다"));
	case EPDModificationResult::NotEnoughGold:
		return FText::FromString(TEXT("골드가 부족합니다"));
	case EPDModificationResult::NotEnoughMaterials:
		return FText::FromString(TEXT("재료가 부족합니다"));
	default:
		return FText::FromString(TEXT("개조 실패"));
	}
}

FText UPDEquipmentModificationWidget::FormatPercent(float Rate) const
{
	return FText::FromString(FString::Printf(TEXT("%.0f%%"), FMath::Clamp(Rate, 0.f, 1.f) * 100.f));
}

FText UPDEquipmentModificationWidget::FormatLevelText(int32 Level) const
{
	return FText::FromString(FString::Printf(TEXT("+%d"), FMath::Max(0, Level)));
}
