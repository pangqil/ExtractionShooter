#include "Core/PDGameMode.h"

#include "Core/PDGameState.h"

APDGameMode::APDGameMode()
{
}

void APDGameMode::StartRaid()
{
	SetRaidState(ERaidState::InProgress);
}

void APDGameMode::RequestExtraction(APlayerController* PC)
{
	if (!PC||CurrentRaidState!=ERaidState::InProgress) return;
	SetRaidState(ERaidState::Extracting);
	EndRaid(true);
}

void APDGameMode::EndRaid(bool bSuccess)
{
	SetRaidState(ERaidState::Ended);
}

void APDGameMode::OnPlayerDied(APlayerController* PC, AActor* Killer)
{
	if (!PC) return;
	EndRaid(false);
}

void APDGameMode::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;
	CurrentRaidState=NewState;
	if (APDGameState* GS=GetGameState<APDGameState>())
	{
		GS->SetRaidState(NewState);
	}
	OnRaidStateChanged(NewState);
}
