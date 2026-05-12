#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Type/Types.h"
#include "PDPlayerController.generated.h"

struct FGameplayTag;
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;
class UPDInputConfig;
class UPDInventoryWidget;
class UPDStashWidget;
class UPDMarketWidget;
class UPDMarketComponent;
class APDPlayerCharacter;
class UPDHUDWidget;
class UPDActivatableBase;
class UPDCrosshairWidget;
class UUserWidget;
class APDWeaponBase;
class APDRifle;

DECLARE_LOG_CATEGORY_EXTERN(LogPDCharacter, Log, All);

UCLASS(abstract)
class PROJECTD_API APDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APDPlayerController();

	UFUNCTION(BlueprintCallable, Category = "PD|Raid")
	void RequestExtraction();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void OpenStashInterface();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void CloseStashInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	bool IsStashInterfaceOpen() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void OpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void CloseMarketInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	bool IsMarketInterfaceOpen() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	bool SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity = 1);

	virtual void PlayerTick(float DeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDHUDWidget> HUDClass;

	virtual void CreateAndAddHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "PD|UI")
	void RequestCloseCurrentScreen();

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UPDInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDStashWidget> StashWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDMarketWidget> MarketWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|UI")
	TSubclassOf<UPDCrosshairWidget> CrosshairWidgetClass;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UseQuickSlot(int32 SlotIndex);

	void OnMove(const struct FInputActionValue& Value);
	void OnJump();
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void UpdateAimRotation();

	void ToggleInventory();
	void TryInteract();

	bool IsGameplayInputBlockedByModalUI() const;
	void SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus = nullptr);

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryWidget> InventoryWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDStashWidget> StashWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketWidget> MarketWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> ActiveMarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDHUDWidget> HUDInstance;

	UPROPERTY()
	TObjectPtr<UPDCrosshairWidget> CrosshairWidget;

	bool bIsGameplayInputBlockedByModalUI = false;
	bool bMouseCursorVisibleBeforeModalUI = false;
	bool bMouseClickEventsEnabledBeforeModalUI = false;
	bool bMouseOverEventsEnabledBeforeModalUI = false;

	void OnFirePressed();
	void OnFireReleased();
	void OnReload();
	void OnInteract();
	void OnSwitchSlot1();
	void OnSwitchSlot2();
	void OnSwitchSlot3();
	void OnUseQuickSlot4();
	void OnZoom();
	void OnToggleFireMode();
	void OnDropWeapon();
	void UpdateCrosshair();

	UFUNCTION()
	void OnWeaponChanged(APDWeaponBase* NewWeapon, EWeaponSlot Slot);
};
