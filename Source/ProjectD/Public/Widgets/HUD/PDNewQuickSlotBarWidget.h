#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDNewQuickSlotBarWidget.generated.h"

class UPanelWidget;
class UHorizontalBox;
class UPDQuickSlotComponent;
class UPDNewQuickSlotItemWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDNewQuickSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void BindQuickSlotComponent(UPDQuickSlotComponent* InQuickSlotComponent);

	UFUNCTION(BlueprintCallable, Category="PD|QuickSlot")
	void RefreshSlots();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot")
	TSubclassOf<UPDNewQuickSlotItemWidget> SlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="1", ClampMax="4"))
	int32 SlotCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="0.0"))
	float SlotSpacing = 16.f;

private:
	UFUNCTION()
	void HandleQuickSlotsChanged();

	void BuildFallbackWidget();
	void RebuildSlotWidgets();
	UPDQuickSlotComponent* FindQuickSlotComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> BoundQuickSlotComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDNewQuickSlotItemWidget>> SlotWidgets;
};
