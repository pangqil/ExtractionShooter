#include "Enemy/AI/BehaviorTree/PDBTService_UpdateCombatBB.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Components/PDCombatComponent.h"

UPDBTService_UpdateCombatBB::UPDBTService_UpdateCombatBB()
{
	NodeName = TEXT("PD Update Combat BB");
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	bNotifyTick = true;
	bTickIntervals = true;
}

void UPDBTService_UpdateCombatBB::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Combat || !BB) return;

	BB->SetValueAsBool (PDBTKeys::CanAttack,    Combat->CanAttack());
	BB->SetValueAsFloat(PDBTKeys::AttackRange,  Combat->GetAttackRange());

	// 시각 타겟이 잡히면 NoiseHint 는 정보가치 낮음 — 자동 만료.
	if (Combat->HasValidTarget() && Combat->HasNoiseHint())
	{
		Combat->ClearNoiseHint();
		BB->SetValueAsBool(PDBTKeys::HasNoiseHint, false);
	}
}
