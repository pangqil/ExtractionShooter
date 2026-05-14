#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Type/Types.h"
#include "PDEquipmentComponent.generated.h"

class UPDInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnEquipmentSlotChanged, EPDEquipmentSlotType, SlotType, FPDInventorySlot, EquippedSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnEquipmentChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDEquipmentComponent();

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment")
	FPDOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment")
	FPDOnEquipmentSlotChanged OnEquipmentSlotChanged;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool EquipItemFromInventory(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment")
	bool UnequipItemToInventory(UPDInventoryComponent* InventoryComponent, EPDEquipmentSlotType SlotType);

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	bool CanEquipItem(const FPDItemData& ItemData) const;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	bool IsSlotOccupied(EPDEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	FPDInventorySlot GetEquippedSlot(EPDEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment")
	EPDEquipmentSlotType ResolveEquipmentSlotType(const FPDItemData& ItemData) const;

	const TMap<EPDEquipmentSlotType, FPDEquippedItem>& GetEquippedItems() const { return EquippedItems; }

protected:
	virtual void BeginPlay() override;

private:
	void InitializeDefaultSlots();
	bool ApplyCharacterEquipSideEffects(const FPDItemData& ItemData) const;
	void RemoveCharacterEquipSideEffects(const FPDItemData& ItemData) const;
	void BroadcastSlotChanged(EPDEquipmentSlotType SlotType);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment", meta = (AllowPrivateAccess = "true"))
	TMap<EPDEquipmentSlotType, FPDEquippedItem> EquippedItems;
};
