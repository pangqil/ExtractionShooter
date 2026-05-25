// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLobbyPlayerController.h"

#include "Core/PDLobbyGameState.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"
#include "Widgets/PDActivatableBase.h"

void APDLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !LobbyScreenClass)
	{
		return;
	}

	if (UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this))
	{
		Subsystem->RequestInitialPush(EUILayer::Frontend, LobbyScreenClass);
	}
}

void APDLobbyPlayerController::Server_SetRoomJoined_Implementation(bool bJoined)
{
	if (UWorld* World = GetWorld())
	{
		if (APDLobbyGameState* GS = World->GetGameState<APDLobbyGameState>())
		{
			GS->SetPlayerJoined(PlayerState, bJoined);
		}
	}
}