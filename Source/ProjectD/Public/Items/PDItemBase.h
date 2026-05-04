#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "Type/Types.h"
#include "PDItemBase.generated.h"

class UDataTable;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDItemBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDItemBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Item")
	UDataTable* ItemDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Item")
	FName ItemRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	FPDItemData CachedItemData;

public:
	virtual void Interact_Implementation(AActor* Interactor) override {}

	FORCEINLINE const FPDItemData& GetItemData() const { return CachedItemData; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Item")
	void OnItemDataLoaded();

private:
	void LoadItemData();
};
