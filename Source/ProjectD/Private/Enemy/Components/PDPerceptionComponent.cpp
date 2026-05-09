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

	// Senior: ConfigureSense 는 *생성자* 시점에 호출되어야 함.
	//         OnRegister() 가 SensesConfig 배열을 훑어 listener 에 sense 들을 매핑하기 때문.
	//         BeginPlay 에서 부르면 listener id 가 없는 상태라 sense 가 매핑되지 않아
	//         자극이 dispatch 되지 않음 (= "Listener must have a valid id" 경고).
	if (SightConfig)
	{
		ConfigureSense(*SightConfig);
		SetDominantSense(SightConfig->GetSenseImplementation());
	}
	if (HearingConfig)
	{
		ConfigureSense(*HearingConfig);
	}
}

void UPDPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Mid: BP defaults 의 값을 SenseConfig 에 반영 — 생성자 시점에는 BP serialize 전이라 적용 불가.
	//      여기서는 ConfigureSense 를 다시 부르지 않음 (이미 생성자에서 등록됨).
	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(SightMaxAge);
		SightConfig->DetectionByAffiliation.bDetectEnemies    = bDetectEnemies;
		SightConfig->DetectionByAffiliation.bDetectNeutrals   = bDetectNeutrals;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->SetMaxAge(HearingMaxAge);
		HearingConfig->DetectionByAffiliation.bDetectEnemies    = bDetectEnemies;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals   = bDetectNeutrals;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	}

	// 변경된 SenseConfig 값을 perception system 에 통지.
	RequestStimuliListenerUpdate();

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
