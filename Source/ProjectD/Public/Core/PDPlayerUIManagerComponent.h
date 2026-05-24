#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
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
class UPDTabbedScreenBase;
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

	// Hub 통합 화면 토글. Input_Inventory 매핑 키가 이쪽으로 일원화됨.
	// 페어링 컨텍스트(Stash/Market/Equip)가 열려있으면 먼저 그것을 닫고 끝남(2단계 close UX).
	// 단독 호출 시 LastHubTabId 기준으로 직전 활성 탭을 복원.
	void ToggleHub();
	bool IsHubOpen() const;

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

	// Hub 통합 화면 (Loadout/Character/Skill/Quest). BP_PDPlayerController 컴포넌트 디폴트에서 WBP_PlayerHubScreen 할당.
	UPROPERTY(EditDefaultsOnly, Category="PD|UI|Hub")
	TSubclassOf<UPDTabbedScreenBase> HubScreenClass;

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

	// Hub 마지막 활성 탭. 닫을 때 캡처 → 다음 열 때 그 탭부터.
	FGameplayTag LastHubTabId;
};
