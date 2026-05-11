#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDQuickSlotBarWidget.generated.h"

class UPanelWidget;
class UPDQuickSlotItemWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDQuickSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 슬롯 개수를 변경하고, 내부 슬롯 위젯을 재생성
	UFUNCTION(BlueprintCallable, Category = "PD|QuickSlot")
	void SetSlotCount(int32 InSlotCount);

	// 선택 슬롯 인덱스를 설정하고, 각 슬롯에 선택 상태를 반영
	UFUNCTION(BlueprintCallable, Category = "PD|QuickSlot")
	void SetSelectedIndex(int32 InIndex);
	
	UFUNCTION(BlueprintPure, Category = "PD|QuickSlot")
	int32 GetSelectedIndex() const { return SelectedIndex; }
	
	UFUNCTION(BlueprintCallable, Category = "PD|QuickSlot")
	void SetSlotSpacing(float InSpacing);

protected:
	virtual void NativeOnInitialized() override;

	// 슬롯 위젯들이 들어갈 컨테이너
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	// 개별 슬롯 위젯의 클래스
	UPROPERTY(EditDefaultsOnly, Category = "PD|QuickSlot")
	TSubclassOf<UPDQuickSlotItemWidget> SlotWidgetClass;

	// 기본 슬롯 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|QuickSlot", meta = (ClampMin = "0"))
	int32 SlotCount = 4;

	// 슬롯 사이 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|QuickSlot", meta = (ClampMin = "0.0"))
	float SlotSpacing = 8.f;

private:
	// SlotCount만큼 슬롯 위젯을 재구성
	void RebuildSlots();
	// 선택 인덱스를 모든 슬롯에 반영
	void ApplySelection();
	// 생성된 슬롯들의 PanelSlot Padding 을 SlotSpacing 기준으로 갱신
	void ApplySlotSpacing();

	// 생성된 슬롯 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDQuickSlotItemWidget>> SlotWidgets;
	
	int32 SelectedIndex = INDEX_NONE;
};

