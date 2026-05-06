#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/PDInventoryComponent.h"
#include "PDMarketComponent.generated.h"

USTRUCT(BlueprintType)
struct FPDMarketEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Stock = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OverridePrice = -1;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDMarketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDMarketComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Market")
	TArray<FPDMarketEntry> Goods;

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool BuyItem(UPDInventoryComponent* BuyerInventory, FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Market")
	bool SellItem(UPDInventoryComponent* SellerInventory, FName ItemID, int32 Quantity = 1);

private:
	FPDMarketEntry* FindEntry(FName ItemID);
};
