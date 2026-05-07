// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Characters/PDBaseNoiseEmitter.h"
#include "Components/StateTreeComponent.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

// Sets default values
APDBaseNoiseEmitter::APDBaseNoiseEmitter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BaseMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComponent"));
	RootComponent = BaseMeshComponent;

	DummyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMeshComponent"));
	DummyMeshComponent->SetupAttachment(RootComponent);

	PerceptionStimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));
	// Hearing 만 등록하면 Soldier 의 Sight 자극이 발생하지 않아 Combat state 진입 불가 → Sight 도 함께 등록.
	PerceptionStimuliSourceComponent->RegisterForSense(UAISense_Hearing::StaticClass());
	PerceptionStimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
}

void APDBaseNoiseEmitter::EmitNoise()
{
	const auto NoiseRange = MaxNoiseRange + FMath::RandRange(-NoiseRangeRandomDeviation, NoiseRangeRandomDeviation);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this,NoiseRange, NoiseTag);
}

// Called when the game starts or when spawned
void APDBaseNoiseEmitter::BeginPlay()
{
	Super::BeginPlay();
	PerceptionStimuliSourceComponent->RegisterWithPerceptionSystem();
	StateTreeComponent->StartLogic();

	// MaxHealth 가 BP 에서 변경됐을 수 있으므로 BeginPlay 시점에 초기화.
	CurrentHealth = MaxHealth;
}

// Called every frame
void APDBaseNoiseEmitter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APDBaseNoiseEmitter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = GetActorLocation() + FVector(0.f, 0.f, SightOffsetZ);
	OutRotation = GetActorRotation();
}

void APDBaseNoiseEmitter::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	if (CurrentHealth <= 0.f)
	{
		return;
	}

	const float Before = CurrentHealth;
	CurrentHealth = FMath::Max(0.f, CurrentHealth - DamageInfo.BaseDamage);

	UE_LOG(LogPDAI, Log, TEXT("[%s] ApplyDamage: %.1f, HP %.1f -> %.1f%s"),
		*GetNameSafe(this),
		DamageInfo.BaseDamage,
		Before,
		CurrentHealth,
		(CurrentHealth <= 0.f) ? TEXT(" [DEAD]") : TEXT(""));

	if (CurrentHealth <= 0.f)
	{
		// Junior: 시각적 피드백만 — 액터 destroy 는 perception 측 stale stimulus 를 만들 수 있어
		//         테스트 단계에서는 가시성/충돌만 끄고 액터는 유지.
		if (BaseMeshComponent)
		{
			BaseMeshComponent->SetVisibility(false);
			BaseMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (DummyMeshComponent)
		{
			DummyMeshComponent->SetVisibility(false);
		}
	}
}

