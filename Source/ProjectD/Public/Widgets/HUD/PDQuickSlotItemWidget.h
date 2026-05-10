#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDQuickSlotItemWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDQuickSlotItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|QuickSlot")
	void SetSelected(bool bInSelected);
	
	UFUNCTION(BlueprintPure, Category = "PD|QuickSlot")
	bool IsSelected() const { return bSelected; }

protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 비선택 상태의 렌더 스케일(기본값 1.0)
	UPROPERTY(EditDefaultsOnly, Category = "PD|QuickSlot|Visual")
	FVector2D NormalScale = FVector2D(1.f, 1.f);

	// 선택 상태의 렌더 스케일
	UPROPERTY(EditDefaultsOnly, Category = "PD|QuickSlot|Visual")
	FVector2D SelectedScale = FVector2D(1.1f, 1.1f);

	// 스케일 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "PD|QuickSlot|Visual")
	float ScaleInterpSpeed = 12.f;

	// 선택 시 재생할 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> SelectPulseAnimation;

private:
	// 현재 선택 상태를 UI에 반영
	void ApplySelectedState();

	// 현재 선택 여부
	bool bSelected = false;

	// 스케일 보간 상태
	FVector2D CurrentScale = FVector2D(1.f, 1.f);
	FVector2D TargetScale = FVector2D(1.f, 1.f);
};
