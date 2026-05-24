// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Lobby/PDPlayModeSelectScreenWidget.h"

#include "Components/Button.h"
#include "Core/PDGameInstance.h"
#include "Core/PDSessionService.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

void UPDPlayModeSelectScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this, &UPDPlayModeSelectScreenWidget::HandleHostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &UPDPlayModeSelectScreenWidget::HandleJoinClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UPDPlayModeSelectScreenWidget::HandleBackClicked);
	}
}

void UPDPlayModeSelectScreenWidget::NativeOnDeactivated()
{
	if (Button_Host)
	{
		Button_Host->OnClicked.RemoveDynamic(this, &UPDPlayModeSelectScreenWidget::HandleHostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.RemoveDynamic(this, &UPDPlayModeSelectScreenWidget::HandleJoinClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UPDPlayModeSelectScreenWidget::HandleBackClicked);
	}

	Super::NativeOnDeactivated();
}

UWidget* UPDPlayModeSelectScreenWidget::GetDesiredFocusTarget_Implementation() const
{
	return Button_Host;
}

void UPDPlayModeSelectScreenWidget::HandleHostClicked()
{
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}
	UPDSessionService* Service = GI->GetSessionService();
	if (!Service)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDPlayModeSelectScreenWidget: SessionService unavailable"));
		return;
	}

	Service->HostSession(MaxLobbyPlayers, GI->GetLobbyLevel());
}

void UPDPlayModeSelectScreenWidget::HandleJoinClicked()
{
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI)
	{
		return;
	}
	UPDSessionService* Service = GI->GetSessionService();
	if (!Service)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDPlayModeSelectScreenWidget: SessionService unavailable"));
		return;
	}

	Service->JoinSession(GetOwningPlayer());
}

void UPDPlayModeSelectScreenWidget::HandleBackClicked()
{
	if (UPDFrontendUISubsystem* Sub = UPDFrontendUISubsystem::Get(this))
	{
		Sub->PopFromLayer(EUILayer::Frontend);
	}
}