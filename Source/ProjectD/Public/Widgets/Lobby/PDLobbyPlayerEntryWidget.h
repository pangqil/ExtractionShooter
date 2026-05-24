// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDLobbyPlayerEntryWidget.generated.h"

class UTextBlock;
class APlayerState;

/**
 * Lobby PlayerList에 들어가는 한 줄짜리 엔트리.
 * SetPlayerState로 대상 PS를 주입받고 PlayerName을 표시한다.
 * 클라이언트에서 PlayerArray는 도착했지만 PlayerName이 아직 빈 문자열인 케이스를 대비해
 * 짧은 폴링으로 보정한다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLobbyPlayerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlayerState(APlayerState* InPlayerState);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_PlayerName;

private:
	UFUNCTION()
	void RefreshName();

	UPROPERTY()
	TWeakObjectPtr<APlayerState> CachedPlayerState;

	FTimerHandle PollTimerHandle;
	int32 PollAttempts = 0;
};