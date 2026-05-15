#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/PDMarketComponent.h"
#include "PDMarketItemWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UWidget;
class UUserWidget;
class UPDInventoryComponent;
class UPDMarketComponent;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMarketItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetMarketEntry(UPDMarketComponent* InMarketComponent, UPDInventoryComponent* InBuyerInventory, const FPDMarketEntry& InEntry, int32 InEntryIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Debug")
	void SetShowLockedRequirementText(bool bInShow);

	UFUNCTION(BlueprintPure, Category = "PD|Market|Debug")
	bool GetShowLockedRequirementText() const { return bShowLockedRequirementText; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName ImageItemIconWidgetName = TEXT("Image_ItemIcon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextItemNameWidgetName = TEXT("Text_ItemName");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextPriceWidgetName = TEXT("Text_Price");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextStockWidgetName = TEXT("Text_Stock");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName ButtonBuyWidgetName = TEXT("Button_Buy");

	// 잠긴 아이템 옆에 필요 마켓 레벨을 표시할 TextBlock 이름입니다.
	// WBP에 Text_RequiredTraderLevel을 추가하면 자동 연결됩니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FName TextRequiredTraderLevelWidgetName = TEXT("Text_RequiredTraderLevel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Widget", meta = (AllowPrivateAccess = "true"))
	FString LockedItemNameString = TEXT("????");

	// true면 잠긴 미리보기 아이템에 필요한 마켓 레벨을 WBP 텍스트로 표시합니다.
	// 별도 잠금 툴팁과 별개로, 화면에 직접 표시할 때만 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Market|Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowLockedRequirementText = false;

	// 잠긴 아이템(????)에 마우스를 올렸을 때 표시할 전용 툴팁 WBP입니다.
	// 예: WBP_LockedMarketTooltip
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Tooltip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> LockedTooltipWidgetClass;

	// LockedTooltipWidgetClass 내부의 필요 마켓 레벨 TextBlock 이름입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Tooltip", meta = (AllowPrivateAccess = "true"))
	FName LockedTooltipRequiredLevelTextWidgetName = TEXT("Text_RequiredTraderLevel");

private:
	UFUNCTION()
	void HandleBuyClicked();

	void ResolveWidgets();
	void RefreshVisuals();
	void ClearMarketTooltips();
	UUserWidget* CreateLockedRequirementTooltipWidget(const FText& RequirementText) const;
	int32 GetUnitPrice() const;
	bool IsLocked() const;
	int32 GetRequiredTraderLevel() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryComponent> BuyerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ImageItemIconWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextItemNameWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextPriceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextStockWidget;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ButtonBuyWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextRequiredTraderLevelWidget;

	FPDMarketEntry Entry;
	int32 EntryIndex = INDEX_NONE;
};
