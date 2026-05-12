#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "Widgets/Inventory/PDInventoryDragDropOperation.h"
#include "PDNewQuickSlotItemWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UPDQuickSlotComponent;
class UPDInventoryComponent;
class UPDStashComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDNewQuickSlotItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void InitializeQuickSlot(UPDQuickSlotComponent* InQuickSlotComponent, int32 InSlotIndex);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void SetSlotData(const FPDInventorySlot& InSlotData);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void ClearSlotData();

	UFUNCTION(BlueprintPure, Category="PD|QuickSlot")
	int32 GetSlotIndex() const { return SlotIndex; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UBorder> SlotBackground;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot")
	FVector2D SlotSize = FVector2D(72.f, 72.f);

private:
	void BuildFallbackWidget();
	void RefreshVisuals();
	UPDInventoryComponent* FindInventoryComponent() const;
	UPDStashComponent* FindStashComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> QuickSlotComponent;

	FPDInventorySlot SlotData;
	int32 SlotIndex = INDEX_NONE;
};
