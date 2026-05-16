#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "PDNewQuickSlotBarWidget.generated.h"

class UPDQuickSlotComponent;
class UPDNewQuickSlotItemWidget;
class UInputAction;
class UInputMappingContext;
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

	// 디자이너가 WBP의 HBox에 슬롯 인스턴스 6개를 이 이름으로 배치 (Is Variable 체크 필수)
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_0;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_1;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_2;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_3;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_4;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDNewQuickSlotItemWidget> Slot_5;

	// 게임플레이 분기용 (0 ~ WeaponSlotCount-1: 무기, 이후: 소모품)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot", meta=(ClampMin="0", ClampMax="6"))
	int32 WeaponSlotCount = 2;

	// 슬롯 인덱스 → 매핑된 IA (6개 BP에서 채움)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TArray<TObjectPtr<UInputAction>> SlotInputActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|QuickSlot|Input")
	TSoftObjectPtr<UPDKeyIconDataAsset> KeyIconMap;

private:
	UFUNCTION()
	void HandleQuickSlotsChanged();

	UFUNCTION()
	void HandleControlMappingsRebuilt();

	UFUNCTION()
	void HandleSelectionChanged(int32 NewIndex);

	// BindWidget으로 잡힌 Slot_0~5를 SlotWidgets 배열에 모음. 슬롯 인스턴스 생성 X.
	void CollectSlotWidgets();
	void ApplyKeyBindings();
	void ApplySelection(int32 SelectedIndex);
	FKey FindKeyForAction(const UInputAction* Action) const;
	UPDQuickSlotComponent* FindQuickSlotComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuickSlotComponent> BoundQuickSlotComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDNewQuickSlotItemWidget>> SlotWidgets;
};