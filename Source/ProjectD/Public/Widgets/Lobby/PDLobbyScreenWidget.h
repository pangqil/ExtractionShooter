// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDLobbyScreenWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UPDLobbyPlayerEntryWidget;
class APDLobbyGameState;
class AGameStateBase;


/**
 * LobbyLevel(=호스트가 연 방)의 메인 화면.
 * 메뉴 단계(NewGame/Continue/Settings/Quit)는 메인 메뉴(WBP_PDMainMenuScreen)로 분리됨.
 * 이 화면은 방 안 상태 표시 + 호스트의 게임 시작 트리거 전담.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLobbyScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* GetDesiredFocusTarget_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_StartGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Leave;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_PlayerList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_PlayerCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_WaitingHost;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Lobby")
	TSubclassOf<UPDLobbyPlayerEntryWidget> PlayerEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Lobby", meta = (ClampMin = "1"))
	int32 MaxPlayersDisplay = 4;

private:
	UFUNCTION() void HandleStartGameClicked();
	UFUNCTION() void HandleLeaveClicked();
	UFUNCTION() void HandleLobbyPlayersChanged();

	void HandleGameStateSet(AGameStateBase* NewGameState);
	void BindToLobbyGameState(APDLobbyGameState* LobbyGS);
	void RefreshPlayerList();
	void ApplyHostState();
	bool IsLocalPlayerHost() const;

	TWeakObjectPtr<APDLobbyGameState> BoundGameState;
	FDelegateHandle GameStateSetHandle;
};