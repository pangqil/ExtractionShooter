#include "Enemy/AI/BehaviorTree/PDBTTask_ThrowGrenade.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDEliteSoldier.h"

UPDBTTask_ThrowGrenade::UPDBTTask_ThrowGrenade()
{
	NodeName = TEXT("PD Throw Grenade");
	bNotifyTick = true;
}

uint16 UPDBTTask_ThrowGrenade::GetInstanceMemorySize() const
{
	return sizeof(FMemory);
}

EBTNodeResult::Type UPDBTTask_ThrowGrenade::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	UWorld* World = GetWorld();

	APDEliteSoldier* Elite = Cast<APDEliteSoldier>(Pawn);
	if (!Elite || !BB || !World) return EBTNodeResult::Failed;

	const FVector TargetLoc = BB->GetValueAsVector(PDBTKeys::LastSeenLocation);

	Elite->ThrowGrenadeAt(TargetLoc);

	// 쿨다운 시작: 다음 틱부터 TrackLOSOcclusion 서비스가 bCanThrowGrenade 를 false 로 유지.
	BB->SetValueAsFloat(PDBTKeys::LastGrenadeTime, World->GetTimeSeconds());
	BB->SetValueAsBool(PDBTKeys::bCanThrowGrenade, false);
	BB->SetValueAsFloat(PDBTKeys::TimeSinceLastLOS, 0.f);

	if (PostThrowDelay <= KINDA_SMALL_NUMBER)
	{
		return EBTNodeResult::Succeeded;
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	Mem->RemainingTime = PostThrowDelay;
	return EBTNodeResult::InProgress;
}

void UPDBTTask_ThrowGrenade::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	Mem->RemainingTime -= DeltaSeconds;
	if (Mem->RemainingTime <= 0.f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
