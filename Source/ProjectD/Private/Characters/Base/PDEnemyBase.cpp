#include "Characters/Base/PDEnemyBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/PDItemBase.h"
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
	// 충돌/이동/자극원 정리. 시체가 발사체를 막거나 계속 인지되지 않도록.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	if (StimuliSource)
	{
		StimuliSource->UnregisterFromPerceptionSystem();
	}

	// 사망 시 드랍 + 시체 컨테이너. 디자이너가 BP 에서 추가 VFX/사운드는 OnLootDropped 로 확장.
	DropLootOnDeath();
	SpawnCorpseContainer();
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);
	Super::HandleDeath(Killer);
}

void APDEnemyBase::OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure)
{
}

void APDEnemyBase::DropLootOnDeath()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> Spawned;
	const FVector Origin = GetActorLocation();

	for (const FPDLootEntry& Entry : LootTable)
	{
		if (!Entry.ItemClass) continue;
		if (FMath::FRand() > Entry.DropChance) continue;

		const int32 MinQ = FMath::Max(1, Entry.MinQuantity);
		const int32 MaxQ = FMath::Max(MinQ, Entry.MaxQuantity);
		const int32 Quantity = FMath::RandRange(MinQ, MaxQ);

		const FVector Offset = (LootSpawnRadius > 0.f)
			? FMath::VRand() * FMath::FRandRange(0.f, LootSpawnRadius)
			: FVector::ZeroVector;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.Owner = this;

		APDItemBase* Item = World->SpawnActor<APDItemBase>(
			Entry.ItemClass,
			Origin + FVector(Offset.X, Offset.Y, 0.f),
			FRotator::ZeroRotator,
			Params);

		if (Item)
		{
			Item->Quantity = Quantity;
			Spawned.Add(Item);
		}
	}

	if (Spawned.Num() > 0)
	{
		OnLootDropped(Spawned);
	}
}

void APDEnemyBase::SpawnCorpseContainer()
{
	if (!CorpseContainerClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	World->SpawnActor<AActor>(
		CorpseContainerClass,
		GetActorLocation(),
		GetActorRotation(),
		Params);
}
