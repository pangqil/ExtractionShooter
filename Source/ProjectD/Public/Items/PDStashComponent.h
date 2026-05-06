#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/PDInventoryComponent.h"
#include "PDStashComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDStashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStashComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Stash")
	TArray<FPDInventorySlot> StashItems;

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool StoreItem(const FPDItemData& ItemData, int32 Quantity = 1, UPDInventoryComponent* SourceInventory = nullptr);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool TakeItem(FName ItemID, int32 Quantity = 1, UPDInventoryComponent* TargetInventory = nullptr);

	UFUNCTION(BlueprintCallable, Category="PD|Stash")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;
};
