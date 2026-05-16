#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "PDNewQuickSlotBarWidget.generated.h"

class UPanelWidget;
class UHorizontalBox;
class UPDQuickSlotComponent;
class UPDNewQuickSlotItemWidget;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class UPDKeyIconDataAsset;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="1", ClampMax="6"))
	int32 SlotCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="0.0"))
	float SlotSpacing = 16.f;

	// 0 ~ WeaponSlotCount-1: 무기 슬롯, 그 이후: 소모품 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="0"))
	int32 WeaponSlotCount = 2;

	// 무기 그룹과 소모품 그룹 사이의 추가 간격(좌측 패딩)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="0.0"))
	float WeaponConsumableGroupGap = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot")
	FVector2D WeaponSlotSize = FVector2D(96.f, 96.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot")
	FVector2D ConsumableSlotSize = FVector2D(72.f, 72.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Visuals")
	TSoftObjectPtr<UMaterialInterface> WeaponSlotMaterial_Base;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Visuals")
	TSoftObjectPtr<UMaterialInterface> ConsumableSlotMaterial_Base;

	// 무기/소모품 공통 선택 강조 머티리얼 (회전 스윕)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Visuals")
	TSoftObjectPtr<UMaterialInterface> SlotMaterial_Selected;

	// 슬롯 인덱스 → 매핑된 IA. SlotCount와 같은 길이로 BP에서 채움
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TArray<TObjectPtr<UInputAction>> SlotInputActions;

	// IA → FKey 매핑이 들어있는 IMC (예: IMC_Input)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	// FKey → 키캡 이미지 매핑
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TSoftObjectPtr<UPDKeyIconDataAsset> KeyIconMap;

private:
	UFUNCTION()
	void HandleQuickSlotsChanged();

	UFUNCTION()
	void HandleControlMappingsRebuilt();

	UFUNCTION()
	void HandleSelectionChanged(int32 NewIndex);

	void BuildFallbackWidget();
	void RebuildSlotWidgets();
	void ApplyKeyBindings();
	void ApplySelection(int32 SelectedIndex);
	FKey FindKeyForAction(const UInputAction* Action) const;
	UPDQuickSlotComponent* FindQuickSlotComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> BoundQuickSlotComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDNewQuickSlotItemWidget>> SlotWidgets;
};
