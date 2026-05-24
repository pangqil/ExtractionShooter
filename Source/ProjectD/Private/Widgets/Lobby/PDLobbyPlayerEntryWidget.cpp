// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDLobbyPlayerEntryWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

void UPDLobbyPlayerEntryWidget::SetPlayerState(APlayerState* InPlayerState)
{
	CachedPlayerState = InPlayerState;
	PollAttempts = 0;
	RefreshName();
}

void UPDLobbyPlayerEntryWidget::RefreshName()
{
	if (!TextBlock_PlayerName)
	{
		return;
	}

	APlayerState* PS = CachedPlayerState.Get();
	if (!PS)
	{
		TextBlock_PlayerName->SetText(FText::GetEmpty());
		return;
	}

	const FString Name = PS->GetPlayerName();
	if (Name.IsEmpty())
	{
		TextBlock_PlayerName->SetText(NSLOCTEXT("PDLobby", "Connecting", "Connecting..."));

		if (PollAttempts < 10)
		{
			++PollAttempts;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					PollTimerHandle, this, &UPDLobbyPlayerEntryWidget::RefreshName, 0.25f, false);
			}
		}
	}
	else
	{
		TextBlock_PlayerName->SetText(FText::FromString(Name));
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PollTimerHandle);
		}
	}
}

void UPDLobbyPlayerEntryWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}
	Super::NativeDestruct();
}