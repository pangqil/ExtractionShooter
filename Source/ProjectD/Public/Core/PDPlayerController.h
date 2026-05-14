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
class UPDStashComponent;
class UPDMarketWidget;
class UPDMarketComponent;
class UPDQuestWindowWidget;
class APDPlayerCharacter;
class UPDHUDWidget;
class UPDActivatableBase;
class UUserWidget;
class APDWeaponBase;
class APDRifle;
class UPDRootLayout;
class UPDPingInputComponent;
class UPDWorldMapWidget;
enum class EWidgetInputMode : uint8;

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
	void OpenStashInterface(UPDStashComponent* StashSource);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void CloseStashInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	bool IsStashInterfaceOpen() const;

	// 현재 열린 박스의 컴포넌트. stash 인터페이스가 닫혀있으면 nullptr.
	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	FORCEINLINE UPDStashComponent* GetActiveStashComponent() const { return ActiveStashComponent.Get(); }

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void OpenMarketInterface(UPDMarketComponent* MarketComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void CloseMarketInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	bool IsMarketInterfaceOpen() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	bool SellInventorySlotToActiveMarket(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "PD|Quest")
	void OpenQuestInterface();

	UFUNCTION(BlueprintCallable, Category = "PD|Quest")
	void CloseQuestInterface();

	UFUNCTION(BlueprintPure, Category = "PD|Quest")
	bool IsQuestInterfaceOpen() const;

	virtual void PlayerTick(float DeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDHUDWidget> HUDClass;

	/** 레이어드 UI의 루트. BP에서 WBP_RootLayout 지정. 미지정 시 레이어드 API 미작동(legacy 경로는 정상). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDRootLayout> RootLayoutClass;

	virtual void CreateAndAddHUDWidget();

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UPDInputConfig> InputConfig;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ping")
	TObjectPtr<UPDPingInputComponent> PingInputComp;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	TSubclassOf<UPDWorldMapWidget> WorldMapClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDStashWidget> StashWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDMarketWidget> MarketWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|UI")
	TSubclassOf<UPDQuestWindowWidget> QuestWindowWidgetClass;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void UseQuickSlot(int32 SlotIndex);

	void OnMove(const struct FInputActionValue& Value);
	void OnJump();
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void UpdateAimRotation();

	void ToggleInventory();
	void ToggleQuest();
	void TryInteract();
	
	void OnToggleWorldMap();

	bool IsGameplayInputBlockedByModalUI() const;
	void SetGameplayInputBlockedByModalUI(bool bBlocked, UUserWidget* WidgetToFocus = nullptr);

	UPROPERTY(Transient)
	TObjectPtr<UPDInventoryWidget> InventoryWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDStashWidget> StashWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketWidget> MarketWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDQuestWindowWidget> QuestWindowWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDMarketComponent> ActiveMarketComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPDHUDWidget> HUDInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDRootLayout> RootLayoutInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPDWorldMapWidget> WorldMapInstance;

	// OpenStashInterface 시 캐시. 박스가 파괴되어도 TWeakObjectPtr가 자동 무효화.
	TWeakObjectPtr<UPDStashComponent> ActiveStashComponent;

	bool bIsGameplayInputBlockedByModalUI = false;
	bool bMouseCursorVisibleBeforeModalUI = false;
	bool bMouseClickEventsEnabledBeforeModalUI = false;
	bool bMouseOverEventsEnabledBeforeModalUI = false;

	/** Subsystem의 OnEffectiveUIStateChanged 콜백. 입력 모드/시스템 커서/크로스헤어 visibility 일괄 적용. */
	void ApplyEffectiveUIState(EWidgetInputMode Mode);

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
