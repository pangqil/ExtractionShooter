// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLobbyPlayerController.h"

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