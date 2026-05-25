// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDLocalSessionService.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UPDLocalSessionService::HostSession(int32 MaxPlayers, TSoftObjectPtr<UWorld> LobbyLevel)
{
	if (LobbyLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDLocalSessionService::HostSession: LobbyLevel is not set"));
		OnHostComplete.Broadcast(false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDLocalSessionService::HostSession: no World"));
		OnHostComplete.Broadcast(false);
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(World, LobbyLevel, true, TEXT("listen"));
	OnHostComplete.Broadcast(true);
}

void UPDLocalSessionService::JoinSession(APlayerController* LocalPC)
{
	if (!LocalPC)
	{
		OnJoinComplete.Broadcast(false);
		return;
	}

	// PIE/LAN 환경에선 호스트가 같은 머신 → 127.0.0.1로 직접 ClientTravel.
	// Steam OSS 도입 시 UPDSteamSessionService가 friend session 검색 후 JoinSession 처리.
	LocalPC->ClientTravel(TEXT("127.0.0.1"), TRAVEL_Absolute);
	OnJoinComplete.Broadcast(true);
}

void UPDLocalSessionService::DestroySession()
{
	OnDestroyComplete.Broadcast(true);
}