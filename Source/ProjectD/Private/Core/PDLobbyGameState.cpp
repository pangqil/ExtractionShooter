// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLobbyGameState.h"

#include "Net/UnrealNetwork.h"

void APDLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APDLobbyGameState, LobbyVersion);
}

void APDLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (HasAuthority())
	{
		++LobbyVersion;
		OnLobbyPlayersChanged.Broadcast();
	}
}

void APDLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	if (HasAuthority())
	{
		++LobbyVersion;
		OnLobbyPlayersChanged.Broadcast();
	}
}

void APDLobbyGameState::OnRep_LobbyVersion()
{
	OnLobbyPlayersChanged.Broadcast();
}