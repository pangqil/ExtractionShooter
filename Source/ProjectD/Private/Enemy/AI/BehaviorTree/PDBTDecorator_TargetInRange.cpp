#include "Enemy/AI/BehaviorTree/PDBTDecorator_TargetInRange.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"

UPDBTDecorator_TargetInRange::UPDBTDecorator_TargetInRange()
{
	NodeName = TEXT("PD Target In Range");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTDecorator_TargetInRange, TargetActorKey),
		AActor::StaticClass());
	TargetActorKey.SelectedKeyName = PDBTKeys::TargetActor;

	RangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTDecorator_TargetInRange, RangeKey));
	RangeKey.SelectedKeyName = PDBTKeys::AttackRange;
}

bool UPDBTDecorator_TargetInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!BB || !Pawn) return false;

	const AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target) return false;

	const float UseRange = bUseBlackboardKey ? BB->GetValueAsFloat(RangeKey.SelectedKeyName) : Range;
	if (UseRange <= 0.f) return false;

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (UseRange * UseRange);
}

FString UPDBTDecorator_TargetInRange::GetStaticDescription() const
{
	if (bUseBlackboardKey)
	{
		return FString::Printf(TEXT("Target in range (BB.%s)"), *RangeKey.SelectedKeyName.ToString());
	}
	return FString::Printf(TEXT("Target in range (%.0f)"), Range);
}
