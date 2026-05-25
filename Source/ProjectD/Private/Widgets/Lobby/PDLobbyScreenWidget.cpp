// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDLobbyScreenWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDGameInstance.h"
#include "Core/PDLobbyGameState.h"
#include "Core/PDSessionService.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/Lobby/PDLobbyPlayerEntryWidget.h"

void UPDLobbyScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_StartGame)
	{
		Button_StartGame->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleStartGameClicked);
	}
	if (Button_Leave)
	{
		Button_Leave->OnClicked.AddDynamic(this, &UPDLobbyScreenWidget::HandleLeaveClicked);
	}

	ApplyHostState();

	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GS = World->GetGameState())
		{
			BindToLobbyGameState(Cast<APDLobbyGameState>(GS));
		}
		else
		{
			GameStateSetHandle = World->GameStateSetEvent.AddUObject(this, &UPDLobbyScreenWidget::HandleGameStateSet);
		}
	}
}

void UPDLobbyScreenWidget::NativeOnDeactivated()
{
	if (Button_StartGame)
	{
		Button_StartGame->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleStartGameClicked);
	}
	if (Button_Leave)
	{
		Button_Leave->OnClicked.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleLeaveClicked);
	}

	if (APDLobbyGameState* GS = BoundGameState.Get())
	{
		GS->OnLobbyPlayersChanged.RemoveDynamic(this, &UPDLobbyScreenWidget::HandleLobbyPlayersChanged);
	}
	BoundGameState.Reset();

	if (UWorld* World = GetWorld())
	{
		if (GameStateSetHandle.IsValid())
		{
			World->GameStateSetEvent.Remove(GameStateSetHandle);
			GameStateSetHandle.Reset();
		}
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDLobbyScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_StartGame;
}

void UPDLobbyScreenWidget::HandleStartGameClicked()
{
	if (!IsLocalPlayerHost())
	{
		return;
	}

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}

	const TSoftObjectPtr<UWorld> BaseLevel = GI->GetBaseLevel();
	if (BaseLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDLobbyScreenWidget::HandleStartGameClicked: BaseLevel is not set on GameInstance."));
		return;
	}

	GI->TravelToLevel(BaseLevel, /*bMarkBaseResetPending=*/false);
}

void UPDLobbyScreenWidget::HandleLeaveClicked()
{
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}
	UPDSessionService* Service = GI->GetSessionService();
	if (!Service)
	{
		return;
	}

	Service->LeaveSession(GetOwningPlayer(), GI->GetStartupLevel());
}

void UPDLobbyScreenWidget::HandleGameStateSet(AGameStateBase* NewGameState)
{
	if (UWorld* World = GetWorld())
	{
		if (GameStateSetHandle.IsValid())
		{
			World->GameStateSetEvent.Remove(GameStateSetHandle);
			GameStateSetHandle.Reset();
		}
	}
	BindToLobbyGameState(Cast<APDLobbyGameState>(NewGameState));
}

void UPDLobbyScreenWidget::BindToLobbyGameState(APDLobbyGameState* LobbyGS)
{
	if (!LobbyGS)
	{
		return;
	}

	BoundGameState = LobbyGS;
	LobbyGS->OnLobbyPlayersChanged.AddDynamic(this, &UPDLobbyScreenWidget::HandleLobbyPlayersChanged);
	RefreshPlayerList();
}

void UPDLobbyScreenWidget::HandleLobbyPlayersChanged()
{
	RefreshPlayerList();
}

void UPDLobbyScreenWidget::RefreshPlayerList()
{
	if (!VerticalBox_PlayerList || !PlayerEntryClass)
	{
		return;
	}

	APDLobbyGameState* GS = BoundGameState.Get();
	if (!GS)
	{
		return;
	}

	VerticalBox_PlayerList->ClearChildren();

	// 유효 참가자만 추려냄.
	TArray<APlayerState*> ValidPlayers;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (PS && !PS->IsInactive())
		{
			ValidPlayers.Add(PS);
		}
	}

	APlayerState* HostPS = GS->GetHostPlayerState();

	// 고정 MaxPlayersDisplay 슬롯: 앞쪽은 참가자, 나머지는 Empty.
	for (int32 SlotIndex = 0; SlotIndex < MaxPlayersDisplay; ++SlotIndex)
	{
		UPDLobbyPlayerEntryWidget* Entry = CreateWidget<UPDLobbyPlayerEntryWidget>(this, PlayerEntryClass);
		if (!Entry)
		{
			continue;
		}

		if (SlotIndex < ValidPlayers.Num())
		{
			APlayerState* PS = ValidPlayers[SlotIndex];
			Entry->SetPlayerState(PS, /*bIsHost=*/PS == HostPS);
		}
		else
		{
			Entry->SetEmpty();
		}

		VerticalBox_PlayerList->AddChild(Entry);
	}

	if (TextBlock_PlayerCount)
	{
		TextBlock_PlayerCount->SetText(
			FText::FromString(FString::Printf(TEXT("%d/%d"), ValidPlayers.Num(), MaxPlayersDisplay)));
	}
}

void UPDLobbyScreenWidget::ApplyHostState()
{
	const bool bHost = IsLocalPlayerHost();

	if (Button_StartGame)
	{
		Button_StartGame->SetIsEnabled(bHost);
	}
	if (TextBlock_WaitingHost)
	{
		TextBlock_WaitingHost->SetVisibility(
			bHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

bool UPDLobbyScreenWidget::IsLocalPlayerHost() const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		return PC->HasAuthority();
	}
	return false;
}