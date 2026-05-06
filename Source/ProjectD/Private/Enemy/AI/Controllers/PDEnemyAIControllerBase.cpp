#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"

#include "Components/StateTreeAIComponent.h"
#include "Enemy/Components/PDPerceptionComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Enemy/Characters/PDBipedEnemy.h"

APDEnemyAIControllerBase::APDEnemyAIControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 부모(AAIController)의 PerceptionComponent UPROPERTY 슬롯에 우리 파생 컴포넌트를 주입.
	// 같은 이름의 멤버를 자식 클래스에서 다시 선언하면 UHT shadowing 오류가 나므로
	// 별도 멤버를 두지 않고 부모 멤버를 그대로 사용한다.
	PerceptionComponent = CreateDefaultSubobject<UPDPerceptionComponent>(TEXT("PDPerception"));

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
}

UPDPerceptionComponent* APDEnemyAIControllerBase::GetPDPerception() const
{
	return Cast<UPDPerceptionComponent>(PerceptionComponent);
}

void APDEnemyAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Senior: Perception 이벤트 구독은 OnPossess 시점에서. Controller 가 미possess 인 동안
	//         감지된 자극은 의미가 없으므로.
	if (UPDPerceptionComponent* PDPerception = GetPDPerception())
	{
		PDPerception->OnTargetSpotted.AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetSpotted);
		PDPerception->OnTargetLost.AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetLost);
		PDPerception->OnNoiseHeard.AddDynamic(this, &APDEnemyAIControllerBase::HandleNoiseHeard);
	}

	// StateTree 시작은 StateTreeAIComponent 가 자동 처리(StartLogic) 또는 디자이너가 BP 에서 호출.
	// 여기서는 자동 시작 시도 — bStartLogicAutomatically 가 false 인 경우를 대비해 명시적 호출.
	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StartLogic();
	}
}

void APDEnemyAIControllerBase::OnUnPossess()
{
	if (UPDPerceptionComponent* PDPerception = GetPDPerception())
	{
		PDPerception->OnTargetSpotted.RemoveAll(this);
		PDPerception->OnTargetLost.RemoveAll(this);
		PDPerception->OnNoiseHeard.RemoveAll(this);
	}

	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StopLogic(TEXT("OnUnPossess"));
	}

	Super::OnUnPossess();
}

void APDEnemyAIControllerBase::HandleTargetSpotted(AActor* Target)
{
	// Senior: AIController 가 perception → CombatComponent 결합 책임을 진다.
	//         이렇게 해야 StateTree 는 단순히 HasValidTarget 만 보고 분기 가능 — perception 이벤트
	//         자체를 StateTree 에서 구독하지 않아 그래프가 단순해짐.
	if (APDBipedEnemy* Biped = Cast<APDBipedEnemy>(GetPawn()))
	{
		if (UPDCombatComponent* Combat = Biped->GetCombatComponent())
		{
			Combat->SetCurrentTarget(Target);
		}
	}

	OnTargetSpotted(Target);
}

void APDEnemyAIControllerBase::HandleTargetLost(AActor* Target, FVector LastKnownLocation)
{
	// Mid: 잃어버린 타겟이 현재 CombatComponent 의 타겟과 동일한 경우에만 Clear.
	//      두 자극원을 동시에 추적하다 한쪽만 Lost 됐을 때 다른 타겟이 날아가지 않도록.
	if (APDBipedEnemy* Biped = Cast<APDBipedEnemy>(GetPawn()))
	{
		if (UPDCombatComponent* Combat = Biped->GetCombatComponent())
		{
			if (Combat->GetCurrentTarget() == Target)
			{
				Combat->ClearCurrentTarget();
			}
		}
	}

	OnTargetLost(Target, LastKnownLocation);
}

void APDEnemyAIControllerBase::HandleNoiseHeard(AActor* NoiseInstigator, FVector Location)
{
	OnNoiseHeard(NoiseInstigator, Location);
}
