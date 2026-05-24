// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDPlayModeSelectScreenWidget.generated.h"

class UButton;

/**
 * NewGame 다음 단계: 호스트로 방을 만들지, 호스트의 방에 참가할지 선택.
 *   - Host: SessionService.HostSession() → OpenLevel(LobbyMap?listen)
 *   - Join: SessionService.JoinSession() → ClientTravel(현재는 127.0.0.1)
 *   - Back: Frontend 레이어 pop → 메인 메뉴 복귀
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPlayModeSelectScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* GetDesiredFocusTarget_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Menu", meta = (ClampMin = "1"))
	int32 MaxLobbyPlayers = 4;

private:
	UFUNCTION() void HandleHostClicked();
	UFUNCTION() void HandleJoinClicked();
	UFUNCTION() void HandleBackClicked();
};