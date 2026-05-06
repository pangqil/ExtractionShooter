#include "Enemy/Components/PDPerceptionComponent.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

UPDPerceptionComponent::UPDPerceptionComponent()
{
	// Mid: SenseConfig는 SubObject로 생성해야 EditDefaultsOnly 값이 BP에서 보임.
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("PDSightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("PDHearingConfig"));
}

void UPDPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Sight 설정.
	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(SightMaxAge);
		SightConfig->DetectionByAffiliation.bDetectEnemies    = bDetectEnemies;
		SightConfig->DetectionByAffiliation.bDetectNeutrals   = bDetectNeutrals;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;

		ConfigureSense(*SightConfig);
		SetDominantSense(SightConfig->GetSenseImplementation());
	}

	// Hearing 설정.
	if (HearingConfig)
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->SetMaxAge(HearingMaxAge);
		HearingConfig->DetectionByAffiliation.bDetectEnemies    = bDetectEnemies;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals   = bDetectNeutrals;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;

		ConfigureSense(*HearingConfig);
	}

	// 원시 perception 이벤트 → 도메인 이벤트 변환.
	OnTargetPerceptionUpdated.AddDynamic(this, &UPDPerceptionComponent::HandlePerceptionUpdated);
}

void UPDPerceptionComponent::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	OnTargetSensed.Broadcast(Actor, Stimulus.WasSuccessfullySensed());

	const FAISenseID SenseID = Stimulus.Type;
	const FAISenseID SightID = UAISense::GetSenseID(UAISense_Sight::StaticClass());
	const FAISenseID HearingID = UAISense::GetSenseID(UAISense_Hearing::StaticClass());

	if (SenseID == SightID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			OnTargetSpotted.Broadcast(Actor);
		}
		else
		{
			// 시야 상실 — 마지막 위치 전달 (Chase → 마지막 위치 이동 로직에 사용).
			OnTargetLost.Broadcast(Actor, Stimulus.StimulusLocation);
		}
	}
	else if (SenseID == HearingID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			OnNoiseHeard.Broadcast(Actor, Stimulus.StimulusLocation);
		}
	}
}
