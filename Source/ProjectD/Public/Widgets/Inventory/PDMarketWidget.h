#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDMarketWidget.generated.h"

class UUniformGridPanel;
class UPDMarketComponent;
class UPDInventoryComponent;
class UUserWidget;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMarketWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void InitializeMarket(UPDMarketComponent* InMarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void RefreshMarketGoods();

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void RefreshMarketInfo();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	TSubclassOf<UUserWidget> MarketItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	FName MarketGridWidgetName = TEXT("UniformGridPanel_MarketGoods");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	int32 MarketGridColumns = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	FName MarketLevelTextWidgetName = TEXT("Text_MarketLevel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market")
	FName MarketExpTextWidgetName = TEXT("Text_MarketExp");

private:
	void ResolveMarketGridPanel();
	void ResolveMarketInfoTextBlocks();

	UFUNCTION()
	void HandleMarketReputationChanged(int32 NewLevel, int32 NewExp);
	UPDInventoryComponent* FindInventoryComponent() const;
	void BindMarketChanged();
	void UnbindMarketChanged();

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> MarketGridPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextMarketLevel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextMarketExp;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> BoundMarketComponent;
};
