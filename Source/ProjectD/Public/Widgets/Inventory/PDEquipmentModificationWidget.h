#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "Items/PDEquipmentModificationComponent.h"
#include "PDEquipmentModificationWidget.generated.h"

class UButton;
class UImage;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UPDInventoryComponent;
class UPDInventorySlotWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDEquipmentModificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshEquipmentList();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void RefreshPreview();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SelectInventorySlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	void SetBoostType(EPDModificationBoostType BoostType);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	TSubclassOf<UPDInventorySlotWidget> EquipmentSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ScrollBoxEquipmentListWidgetName = TEXT("ScrollBox_EquipmentList");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ImageSelectedItemIconWidgetName = TEXT("Image_SelectedItemIcon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextSelectedItemNameWidgetName = TEXT("Text_SelectedItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextCurrentLevelWidgetName = TEXT("Text_CurrentLevel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextTargetLevelWidgetName = TEXT("Text_TargetLevel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextStatTypeWidgetName = TEXT("Text_StatType");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextStatPreviewWidgetName = TEXT("Text_StatPreview");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextGoldCostWidgetName = TEXT("Text_GoldCost");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName VerticalBoxRequiredMaterialsWidgetName = TEXT("VerticalBox_RequiredMaterials");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextBaseSuccessRateWidgetName = TEXT("Text_BaseSuccessRate");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextBoostSuccessRateWidgetName = TEXT("Text_BoostSuccessRate");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextFinalSuccessRateWidgetName = TEXT("Text_FinalSuccessRate");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ButtonBoostNoneWidgetName = TEXT("Button_Boost_None");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ButtonBoostLowWidgetName = TEXT("Button_Boost_Low");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ButtonBoostMidWidgetName = TEXT("Button_Boost_Mid");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ButtonBoostHighWidgetName = TEXT("Button_Boost_High");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName ButtonModifyWidgetName = TEXT("Button_Modify");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Widget")
	FName TextResultWidgetName = TEXT("Text_Result");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText EmptySelectionText = FText::FromString(TEXT("장비를 선택하세요"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Format")
	FText ModifyReadyText = FText::FromString(TEXT("개조를 시도하면 결과가 표시됩니다."));

private:
	void ResolveComponents();
	void ResolveWidgets();
	void BindWidgetEvents();
	void UnbindWidgetEvents();
	void BindComponentEvents();
	void UnbindComponentEvents();
	void SelectFirstEquipmentSlotIfNeeded();
	void ClearPreview();
	void ApplyPreview(const FPDModificationPreview& Preview, const FPDInventorySlot& SlotData);
	void RefreshMaterialList(const FPDModificationPreview& Preview);
	void SetText(UTextBlock* TextBlock, const FText& Text) const;
	void SetTextByName(FName WidgetName, const FText& Text) const;
	int32 CountItem(FName ItemID) const;
	FText GetResultText(EPDModificationResult Result, bool bSuccess) const;
	FText FormatPercent(float Rate) const;
	FText FormatLevelText(int32 Level) const;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleModificationFinished(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result);

	UFUNCTION()
	void HandleEquipmentSlotClicked(UPDInventorySlotWidget* SlotWidget, int32 SlotIndex);

	UFUNCTION()
	void HandleBoostNoneClicked();

	UFUNCTION()
	void HandleBoostLowClicked();

	UFUNCTION()
	void HandleBoostMidClicked();

	UFUNCTION()
	void HandleBoostHighClicked();

	UFUNCTION()
	void HandleModifyClicked();

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDEquipmentModificationComponent> ModificationComponent;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ScrollBoxEquipmentList;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ImageSelectedItemIcon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextSelectedItemName;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextCurrentLevel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextTargetLevel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextStatType;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextStatPreview;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextGoldCost;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> VerticalBoxRequiredMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBaseSuccessRate;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBoostSuccessRate;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextFinalSuccessRate;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBoostNone;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBoostLow;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBoostMid;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBoostHigh;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonModify;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextResult;

	int32 SelectedSlotIndex = INDEX_NONE;
	EPDModificationBoostType SelectedBoostType = EPDModificationBoostType::None;
	FPDModificationPreview CurrentPreview;
	EPDModificationResult CurrentPreviewResult = EPDModificationResult::InvalidSlot;
	bool bCurrentPreviewValid = false;
};
