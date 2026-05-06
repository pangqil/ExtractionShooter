#include "Characters/Base/PDEnemyBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

APDEnemyBase::APDEnemyBase()
{
	// Junior: ASC 는 부모(APDCharacterBase) 생성자에서 만들어졌음. nullptr 가드.
	// Mid: replication 모드는 적 AI 라 Minimal 로 충분. (큰 데이터는 호스트만 보유)
	if (ASC)
	{
		ASC->SetIsReplicated(true);
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}

	// 자극원 컴포넌트: 다른 AI가 본 적을 인지하려면 등록되어 있어야 함.
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	if (StimuliSource)
	{
		StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
		StimuliSource->bAutoRegister = true;
	}
}

uint8 APDEnemyBase::GetTeamID_Implementation() const
{
	return TeamID;
}

bool APDEnemyBase::IsAlive_Implementation() const
{
	// Senior: Damage 인터페이스 측 IsAlive 와 동일한 진실 원천(GAS Health)을 사용해
	//         두 인터페이스가 항상 일치하도록.
	return IPDDamageable::Execute_IsAlive(this);
}

EPDBatteryStatus APDEnemyBase::GetBatteryStatus_Implementation() const
{
	// 비-Biped 디폴트. Biped 자식 클래스가 StatComponent로부터 실제 상태 반환.
	return EPDBatteryStatus::None;
}

void APDEnemyBase::SetEnemyState(EPDEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;

	switch (NewState)
	{
	case EPDEnemyState::Idle:   OnEnterState_Idle();   break;
	case EPDEnemyState::Alert:  OnEnterState_Alert();  break;
	case EPDEnemyState::Chase:  OnEnterState_Chase();  break;
	case EPDEnemyState::Combat: OnEnterState_Combat(); break;
	case EPDEnemyState::Dead:   OnEnterState_Dead();   break;
	default: break;
	}

	OnEnemyStateChanged(NewState);
}

void APDEnemyBase::OnEnterState_Dead()
{
	// 충돌 비활성화 — 시체가 발사체를 막거나 플레이어를 밀치지 않도록.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 이동 컴포넌트도 정지.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// 자극원 등록 해제 — 시체가 계속 인지되지 않도록.
	if (StimuliSource)
	{
		StimuliSource->UnregisterFromPerceptionSystem();
	}

	// Junior: 루트박스/시체 액터 스폰은 BP 디자이너가 OnDeath/OnEnemyStateChanged 에서 처리.
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);
	Super::HandleDeath(Killer);
}

void APDEnemyBase::OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure)
{
}
