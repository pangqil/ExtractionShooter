#include "Enemy/AI/BehaviorTree/PDBTDecorator_HasLOS.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Engine/World.h"

UPDBTDecorator_HasLOS::UPDBTDecorator_HasLOS()
{
	NodeName = TEXT("PD Has LOS");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTDecorator_HasLOS, TargetActorKey),
		AActor::StaticClass());
	TargetActorKey.SelectedKeyName = PDBTKeys::TargetActor;
}

bool UPDBTDecorator_HasLOS::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!BB || !Pawn) return false;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_LOS), true, Pawn);
	Params.AddIgnoredActor(Target);

	const FVector Start = Pawn->GetActorLocation();
	const FVector End = Target->GetActorLocation();

	// Visibility 채널이 막히지 않으면 LOS 확보.
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked;
}
