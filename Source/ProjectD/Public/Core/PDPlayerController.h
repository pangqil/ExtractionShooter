#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PDPlayerController.generated.h"

struct FGameplayTag;
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;
class UPDInputConfig;
class APDWeaponBase;
class APDRifle;
class APDPlayerCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogPDCharacter, Log, All);

class UInputMappingContext;
class UPDInputConfig;

UCLASS(abstract)
class PROJECTD_API APDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APDPlayerController();

	UFUNCTION(BlueprintCallable, Category = "PD|Raid")
	void RequestExtraction();

	virtual void PlayerTick(float DeltaTime) override; 

protected:
	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "PD|Input")
	TObjectPtr<UPDInputConfig> InputConfig;

	virtual void SetupInputComponent() override;

private:
	void OnMove(const struct FInputActionValue& Value);
	void OnJump();
	void OnAbilityInputPressed(FGameplayTag InputTag);
	void OnAbilityInputReleased(FGameplayTag InputTag);
	void UpdateAimRotation();
	
	// 무기 입력
	void OnFirePressed();
	void OnFireReleased();
	void OnReload();
	void OnSwitchSlot1();
	void OnSwitchSlot2();
	void OnSwitchSlot3();
	void OnToggleFireMode();
	void OnInteract();
	
	// 현재 무기 가져오기
	APDWeaponBase* GetCurrentWeapon() const;

	// 소유 PlayerCharacter 가져오기
	APDPlayerCharacter* GetPlayerCharacter() const;
	
	bool bShowMouseCursor=true;
};