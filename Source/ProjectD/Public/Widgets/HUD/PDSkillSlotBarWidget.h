#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDSkillSlotBarWidget.generated.h"

class UPanelWidget;
class UTexture2D;
class UPDSkillSlotWidget;

UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDSkillSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void SetSelectedIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category="PD|SkillSlot")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void RebuildSlots();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot", meta=(ClampMin="1"))
	int32 SlotCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot", meta=(ClampMin="0.0"))
	float SlotSpacing = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot")
	TSubclassOf<UPDSkillSlotWidget> SlotWidgetClass;

	// 인덱스별 스킬 아이콘 (SlotCount와 같은 길이로 BP에서 채움)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TArray<TSoftObjectPtr<UTexture2D>> SkillIcons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TSoftObjectPtr<UTexture2D> SelectedTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TSoftObjectPtr<UTexture2D> UnselectedTexture;

private:
	void ApplySelection();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDSkillSlotWidget>> SlotWidgets;

	int32 SelectedIndex = INDEX_NONE;
};