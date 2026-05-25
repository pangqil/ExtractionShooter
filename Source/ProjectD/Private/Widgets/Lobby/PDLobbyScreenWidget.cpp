// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDLobbyScreenWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDGameInstance.h"
#include "Core/PDLobbyGameState.h"
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

	int32 Count = 0;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS || PS->IsInactive())
		{
			continue;
		}

		UPDLobbyPlayerEntryWidget* Entry = CreateWidget<UPDLobbyPlayerEntryWidget>(this, PlayerEntryClass);
		if (Entry)
		{
			Entry->SetPlayerState(PS);
			VerticalBox_PlayerList->AddChild(Entry);
			++Count;
		}
	}

	if (TextBlock_PlayerCount)
	{
		TextBlock_PlayerCount->SetText(
			FText::FromString(FString::Printf(TEXT("%d/%d"), Count, MaxPlayersDisplay)));
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