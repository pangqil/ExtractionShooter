#include "Enemy/AI/BehaviorTree/PDBTService_TrackLOSOcclusion.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UPDBTService_TrackLOSOcclusion::UPDBTService_TrackLOSOcclusion()
{
	NodeName = TEXT("PD Track LOS Occlusion");
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	bNotifyTick = true;
	bTickIntervals = true;
}

void UPDBTService_TrackLOSOcclusion::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	UWorld* World = GetWorld();
	if (!BB || !World) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(PDBTKeys::TargetActor));
	const bool bHasLOS = BB->GetValueAsBool(PDBTKeys::HasLOSToTarget);

	float TimeSince = BB->GetValueAsFloat(PDBTKeys::TimeSinceLastLOS);
	if (Target && bHasLOS)
	{
		TimeSince = 0.f;
		// LOS 확보 동안 LastSeenLocation 을 최신화 — grenade 가 던질 좌표 신뢰도 ↑.
		BB->SetValueAsVector(PDBTKeys::LastSeenLocation, Target->GetActorLocation());
	}
	else if (Target)
	{
		TimeSince += DeltaSeconds;
	}
	else
	{
		TimeSince = 0.f;
	}
	BB->SetValueAsFloat(PDBTKeys::TimeSinceLastLOS, TimeSince);

	const float LastGrenade = BB->GetValueAsFloat(PDBTKeys::LastGrenadeTime);
	const float Now = World->GetTimeSeconds();
	const bool bOffCooldown = (LastGrenade <= 0.f) || ((Now - LastGrenade) >= GrenadeCooldown);

	const bool bCanThrow = (Target != nullptr) && (TimeSince >= OcclusionThreshold) && bOffCooldown;
	BB->SetValueAsBool(PDBTKeys::bCanThrowGrenade, bCanThrow);
}
