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

void APDBipedEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Senior: HP=0 이벤트를 본 클래스가 처리 — HandleDeath 까지 연결.
	//         StatComponent 가 GAS attribute 변화를 감시 → 도메인 이벤트로 변환 → 본 캐릭터 사망 처리.
	if (StatComponent)
	{
		StatComponent->OnHealthDepleted.AddDynamic(this, &APDBipedEnemy::HandleHealthDepleted);
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

void APDBipedEnemy::HandleHealthDepleted()
{
	// Killer 정보가 없으므로 nullptr — 향후 GAS Effect Context 에서 Instigator 추출하여 전달 가능.
	HandleDeath(nullptr);
}
