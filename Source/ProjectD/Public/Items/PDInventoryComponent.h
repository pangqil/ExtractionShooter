#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDInventoryComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Inventory")
	TArray<FPDInventorySlot> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Inventory")
	int32 Gold = 0;

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	bool AddItem(const FPDItemData& ItemData, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	bool UseItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="PD|Inventory")
	bool SpendGold(int32 Amount);

	UFUNCTION(BlueprintPure, Category="PD|Inventory")
	int32 GetGold() const { return Gold; }
};
