#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PDQuestData.h"
#include "PDQuestListItemWidget.generated.h"

class UButton;
class UTextBlock;
class UPDQuestWindowWidget;

UCLASS(Abstract, BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDQuestListItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void SetupQuestItem(UPDQuestWindowWidget* InOwnerWidget, const FPDQuestData& InQuestData, EPDQuestState InState, const FText& InProgressText, bool bInSelected);

	UFUNCTION(BlueprintCallable, Category="PD|Quest")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category="PD|Quest")
	FName GetQuestID() const { return QuestData.QuestID; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> BTN_Select;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_QuestName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_QuestProgress;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Progress;

private:
	UFUNCTION()
	void HandleSelectClicked();
	void UpdateVisualStyle();

	UPROPERTY(Transient)
	TObjectPtr<UPDQuestWindowWidget> OwnerWidget;

	UPROPERTY(Transient)
	FPDQuestData QuestData;

	EPDQuestState QuestState = EPDQuestState::Inactive;
	FText ProgressText;
	bool bSelected = false;
};
