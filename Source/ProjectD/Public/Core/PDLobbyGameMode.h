// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PDLobbyGameMode.generated.h"

/**
 * StartupLevel / LobbyLevel용 GameMode.
 * Lobby 화면 push는 PlayerControllerClass(=APDLobbyPlayerController)가 자체적으로 처리한다.
 * 본 클래스는 PlayerControllerClass 디폴트 지정용 마커 + 향후 lobby 전용 server-side 로직(인원 체크 등) 확장 지점.
 */
UCLASS()
class PROJECTD_API APDLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
};