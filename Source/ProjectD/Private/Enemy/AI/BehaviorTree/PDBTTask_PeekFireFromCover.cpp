#include "Enemy/AI/BehaviorTree/PDBTTask_PeekFireFromCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDEliteSoldier.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

UPDBTTask_PeekFireFromCover::UPDBTTask_PeekFireFromCover()
{
	NodeName = TEXT("PD Peek Fire From Cover (LOS)");
	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;

	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

uint16 UPDBTTask_PeekFireFromCover::GetInstanceMemorySize() const
{
	return sizeof(FPDPeekFireTaskMemory);
}

namespace
{
	bool HasArrived(AAIController* AI, const FVector& Target, float AcceptanceRadius)
	{
		if (!AI) return false;
		APawn* P = AI->GetPawn();
		if (!P) return false;
		const float DistSq = FVector::DistSquared2D(P->GetActorLocation(), Target);
		return DistSq <= AcceptanceRadius * AcceptanceRadius;
	}

	// 폰 → 타겟 LineTrace (Visibility). 차단 없으면 LOS 확보로 간주.
	bool HasLOSToTarget(const APawn* Pawn, const AActor* Target, float Height)
	{
		if (!Pawn || !Target) return false;
		UWorld* World = Pawn->GetWorld();
		if (!World) return false;

		const FVector Start = Pawn->GetActorLocation()   + FVector(0.f, 0.f, Height);
		const FVector End   = Target->GetActorLocation() + FVector(0.f, 0.f, Height);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_PeekLOS), /*bTraceComplex=*/false, Pawn);
		Params.AddIgnoredActor(Target);

		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		return !bBlocked;
	}
}

EBTNodeResult::Type UPDBTTask_PeekFireFromCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Pawn || !Target) return EBTNodeResult::Failed;

	FPDPeekFireTaskMemory* Mem = reinterpret_cast<FPDPeekFireTaskMemory*>(NodeMemory);
	Mem->OriginalLoc      = Pawn->GetActorLocation();
	Mem->RepositionTarget = FVector::ZeroVector;
	Mem->TotalElapsed     = 0.f;
	Mem->SettleElapsed    = 0.f;
	Mem->FireElapsed      = 0.f;
	Mem->LosLostElapsed   = 0.f;
	Mem->bGoLeft          = false;
	Mem->State            = EPDPeekFireState::Settling;

	AI->SetFocus(Target, EAIFocusPriority::Gameplay);

	// PeekSpot 계산은 Settling 종료 시점으로 미룸 (그때 좌/우 랜덤 결정 + 최신 Target 위치 반영).
	return EBTNodeResult::InProgress;
}

void UPDBTTask_PeekFireFromCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	APDEliteSoldier* Elite = Cast<APDEliteSoldier>(Pawn);
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!AI || !Pawn || !Elite || !Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FPDPeekFireTaskMemory* Mem = reinterpret_cast<FPDPeekFireTaskMemory*>(NodeMemory);
	Mem->TotalElapsed += DeltaSeconds;

	// safety timeout — 이동 막힘/스택 방지.
	if (Mem->TotalElapsed >= MaxTotalDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	switch (Mem->State)
	{
	case EPDPeekFireState::Settling:
	{
		Mem->SettleElapsed += DeltaSeconds;
		if (Mem->SettleElapsed < SettleDuration) break;

		// 좌/우 랜덤 결정. 좌측은 muzzle 보정으로 거리 더 길게.
		Mem->bGoLeft = FMath::RandBool();
		const float StrafeDist = Mem->bGoLeft ? PeekOffsetLeft : PeekOffsetRight;

		const FVector ToTarget = (Target->GetActorLocation() - Mem->OriginalLoc) * FVector(1, 1, 0);
		const FVector ToTargetDir = ToTarget.GetSafeNormal();
		if (ToTargetDir.IsNearlyZero())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		// UE 좌수 좌표계: ToTarget × Up = 폰 기준 왼쪽. 우측은 부호 반전.
		const FVector LeftDir  =  FVector::CrossProduct(ToTargetDir, FVector::UpVector);
		const FVector StrafeDir = Mem->bGoLeft ? LeftDir : -LeftDir;

		Mem->PeekSpot = Mem->OriginalLoc + StrafeDir * StrafeDist;

		const EPathFollowingRequestResult::Type MoveResult =
			AI->MoveToLocation(Mem->PeekSpot, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
				/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
				/*bCanStrafe=*/true);

		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
		Mem->State = EPDPeekFireState::Strafing;
		break;
	}

	case EPDPeekFireState::Strafing:
	{
		// 매 tick LOS 확인 — 확보 즉시 strafe 중단 후 사격.
		if (HasLOSToTarget(Pawn, Target, LOSCheckHeight))
		{
			AI->StopMovement();
			Elite->SetPeeking(true);
			Mem->State = EPDPeekFireState::Firing;
			Mem->FireElapsed = 0.f;
			break;
		}

		// LOS 미확보 상태로 PeekSpot 도착 → fallback 으로 그래도 사격 시작.
		if (HasArrived(AI, Mem->PeekSpot, MoveAcceptanceRadius))
		{
			Elite->SetPeeking(true);
			Mem->State = EPDPeekFireState::Firing;
			Mem->FireElapsed = 0.f;
		}
		break;
	}

	case EPDPeekFireState::Firing:
	{
		Mem->FireElapsed += DeltaSeconds;

		// 우선순위 1: 사격 지속시간 또는 장전 시작 → cover 복귀.
		bool bShouldReturn = (Mem->FireElapsed >= FireDuration);
		if (!bShouldReturn && bAbortOnReload)
		{
			if (const APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(Elite->GetEquippedWeapon()))
			{
				if (Ranged->IsReloading())
				{
					bShouldReturn = true;
				}
			}
		}

		if (bShouldReturn)
		{
			Elite->SetPeeking(false);
			AI->MoveToLocation(Mem->OriginalLoc, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
				/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
				/*bCanStrafe=*/true);
			Mem->State = EPDPeekFireState::Returning;
			break;
		}

		// 우선순위 2: 사격 중 LOS 잃음 → grace 시간 후 EQS reposition.
		const bool bHasLOS = HasLOSToTarget(Pawn, Target, LOSCheckHeight);
		if (bHasLOS)
		{
			Mem->LosLostElapsed = 0.f; // LOS 회복 → grace 리셋
			break;
		}

		Mem->LosLostElapsed += DeltaSeconds;
		if (Mem->LosLostElapsed < LosLostTolerance) break;

		// reposition 시도.
		Elite->SetPeeking(false);

		if (RepositionQuery && Pawn->GetWorld())
		{
			FEnvQueryRequest Request(RepositionQuery, Pawn);
			TSharedPtr<FEnvQueryResult> Result =
				UEnvQueryManager::GetCurrent(Pawn->GetWorld())->RunInstantQuery(Request, EEnvQueryRunMode::SingleBest);

			if (Result.IsValid() && Result->IsSuccessful())
			{
				Mem->RepositionTarget = Result->GetItemAsLocation(0);
				AI->MoveToLocation(Mem->RepositionTarget, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
					/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
					/*bCanStrafe=*/true);
				Mem->State = EPDPeekFireState::Repositioning;
				Mem->LosLostElapsed = 0.f;
				break;
			}
		}

		// RepositionQuery 미설정 OR 후보 없음 → cover 복귀로 폴백.
		AI->MoveToLocation(Mem->OriginalLoc, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
			/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
			/*bCanStrafe=*/true);
		Mem->State = EPDPeekFireState::Returning;
		break;
	}

	case EPDPeekFireState::Repositioning:
	{
		if (HasArrived(AI, Mem->RepositionTarget, MoveAcceptanceRadius))
		{
			// 도착 후 LOS 재확인. 잡았으면 사격 재개, 아니면 cover 복귀.
			if (HasLOSToTarget(Pawn, Target, LOSCheckHeight))
			{
				Elite->SetPeeking(true);
				Mem->State = EPDPeekFireState::Firing;
				Mem->FireElapsed = 0.f;       // 새 위치에서 FireDuration 다시 카운트
				Mem->LosLostElapsed = 0.f;
			}
			else
			{
				AI->MoveToLocation(Mem->OriginalLoc, MoveAcceptanceRadius, /*bStopOnOverlap=*/true,
					/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true,
					/*bCanStrafe=*/true);
				Mem->State = EPDPeekFireState::Returning;
			}
		}
		break;
	}

	case EPDPeekFireState::Returning:
	{
		if (HasArrived(AI, Mem->OriginalLoc, MoveAcceptanceRadius))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;
	}
	}
}

EBTNodeResult::Type UPDBTTask_PeekFireFromCover::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::AbortTask(OwnerComp, NodeMemory);
	return EBTNodeResult::Aborted;
}

void UPDBTTask_PeekFireFromCover::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI) return;

	// 진행 중이던 사격 무조건 정지 — 외부 abort 로 끊겨도 발사 루프 잔존 방지.
	if (APDEliteSoldier* Elite = Cast<APDEliteSoldier>(AI->GetPawn()))
	{
		Elite->SetPeeking(false);
	}

	AI->ClearFocus(EAIFocusPriority::Gameplay);
	AI->StopMovement();
}
