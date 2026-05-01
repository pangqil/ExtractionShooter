#include "Characters/PDEnemyBase.h"

APDEnemyBase::APDEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APDEnemyBase::SetEnemyState(EPDEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;
	OnEnemyStateChanged(NewState);
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);
	Super::HandleDeath(Killer);
}
