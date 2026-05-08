#include "Enemy/Characters/PDBipedEnemy.h"

#include "Enemy/Components/PDStatComponent.h"
#include "Enemy/Components/PDCombatComponent.h"

APDBipedEnemy::APDBipedEnemy()
{
	// Mid: 컴포넌트는 생성자에서 CreateDefaultSubobject 로 만들어야
	//     BP 디폴트 / replication / SubObject 트리에 정상 등록됨.
	StatComponent = CreateDefaultSubobject<UPDStatComponent>(TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UPDCombatComponent>(TEXT("CombatComponent"));

	// Biped는 Battery(스태미너) 사용.
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
