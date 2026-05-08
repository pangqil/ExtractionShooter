#include "Enemy/Characters/PDBipedEnemy.h"

#include "Enemy/Components/PDStatComponent.h"
#include "Enemy/Components/PDCombatComponent.h"

APDBipedEnemy::APDBipedEnemy()
{
	StatComponent   = CreateDefaultSubobject<UPDStatComponent>  (TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UPDCombatComponent>(TEXT("CombatComponent"));

	if (StatComponent)
	{
		StatComponent->SetUseBattery(true);
	}
}

EPDBatteryStatus APDBipedEnemy::GetBatteryStatus_Implementation() const
{
	if (StatComponent && StatComponent->IsUsingBattery())
	{
		return StatComponent->GetBatteryStatus();
	}
	return EPDBatteryStatus::None;
}
