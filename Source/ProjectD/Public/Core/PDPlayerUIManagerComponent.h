#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Type/Types.h"
#include "PDPlayerUIManagerComponent.generated.h"

class APDPlayerController;
class APDWeaponBase;
class UAbilitySystemComponent;
class UPDHUDWidget;
class UPDInventoryWidget;
class UPDMarketComponent;
class UPDMarketWidget;
class UPDNotificationWidget;
class UPDQuestWindowWidget;
class UPDRootLayout;
class UPDStashComponent;
class UPDStashWidget;
class UPDWorldMapWidget;
class UUserWidget;

enum class EWidgetInputMode : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDPlayerUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDPlayerUIManagerComponent();

	void InitializeUI(APDPlayerController* InOwnerController);
	void ShutdownUI(const EEndPlayReason::Type EndPlayReason);

	void ConfigureLegacyWidgetClasses(
		TSubclassOf<UPDHUDWidget> InHUDClass,
		TSubclassOf<UPDRootLayout> InRootLayoutClass,
		TSubclassOf<UPDInventoryWidget> InInventoryWidgetClass,
		TSubclassOf<UPDStashWidget> InStashWidgetClass,
		TSubclassOf<UPDMarketWidget> InMarketWidgetClass,
		TSubclassOf<UUserWidget> InEquipmentModificationWidgetClass,
		TSubclassOf<UPDNotificationWidget> InNotificationWidgetClass,
		float InNotificationDuration,
		int32 InNotificationZOrder,
		TSubclassOf<UPDQuestWindowWidget> InQuestWindowWidgetClass,
		TSubclassOf<UPDWorldMapWidget> InWorldMapClass);

	void CreateAndAddHUDWidget();
	void RebindHUDToASC(UAbilitySystemComponent* ASC);
	void ApplyEffectiveUIState(EWidgetInputMode Mode);

	void OpenStash(UPDStashComponent* StashSource);
	void CloseStash();
	bool IsStashOpen() const;
	UPDStashComponent* GetActiveStashComponent() const { return ActiveStashComponent.Get(); }

	void OpenMarket(UPDMarketComponent* MarketComponent);
	void CloseMarket();
	bool IsMarketOpen() const;
	UPDMarketComponent* GetActiveMarketComponent() const { return ActiveMarketComponent.Get(); }

	void OpenEquipmentModification();
	void CloseEquipmentModification();
	bool IsEquipmentModificationOpen() const;

	void OpenQuest();
	void CloseQuest();
	bool IsQuestOpen() const;
	void ToggleQuest();

	void ToggleInventory();

	void ShowNotification(const FText& Message, float Duration = 2.f);
	void ToggleWorldMap();

	bool IsGameplayInputBlockedByModalUI() const { return bIsGameplayInputBlockedByModalUI; }
	bool ShouldAllowMovementWhileUIOpen() const;
	void SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus = nullptr);

	void RefreshNewQuickSlots();
	void UpdateCrosshair(const FVector2D& MousePosition, float Spread);
	void SetCrosshairType(APDWeaponBase* NewWeapon);

private:
	APDPlayerController* GetPDController() const;
	void AddWidgetToViewportIfNeeded(UUserWidget* Widget, int32 ZOrder = 0) const;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDHUDWidget> HUDClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDRootLayout> RootLayoutClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDStashWidget> StashWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDMarketWidget> MarketWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UUserWidget> EquipmentModificationWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDNotificationWidget> NotificationWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI", meta=(ClampMin="0.1"))
	float NotificationDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	int32 NotificationZOrder = 50;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDQuestWindowWidget> QuestWindowWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="PD|UI")
	TSubclassOf<UPDWorldMapWidget> WorldMapClass;

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryWidget> InventoryWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDStashWidget> StashWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketWidget> MarketWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> EquipmentModificationWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDNotificationWidget> NotificationWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuestWindowWidget> QuestWindowWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDHUDWidget> HUDInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDRootLayout> RootLayoutInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDWorldMapWidget> WorldMapInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> ActiveMarketComponent;

	TWeakObjectPtr<UPDStashComponent> ActiveStashComponent;

	bool bIsGameplayInputBlockedByModalUI = false;
	bool bMouseCursorVisibleBeforeModalUI = false;
	bool bMouseClickEventsEnabledBeforeModalUI = false;
	bool bMouseOverEventsEnabledBeforeModalUI = false;
};
