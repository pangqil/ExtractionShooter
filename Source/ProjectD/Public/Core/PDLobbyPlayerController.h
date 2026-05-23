// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/PDPlayerController.h"
#include "PDLobbyPlayerController.generated.h"

class UPDActivatableBase;

/**
 * Lobby/StartupLevel 전용 PlayerController.
 * BeginPlay 시 자체 LobbyScreenClass를 Frontend 레이어에 push 요청.
 * PC-driven이라 서버/클라이언트 양쪽 인스턴스가 각자 local PC BeginPlay에서 push가 실행됨 —
 * PIE Listen Server N≥2 모드에서 호스트와 클라이언트 모두 lobby UI를 보게 된다.
 */
UCLASS()
class PROJECTD_API APDLobbyPlayerController : public APDPlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|Lobby")
	TSubclassOf<UPDActivatableBase> LobbyScreenClass;
};