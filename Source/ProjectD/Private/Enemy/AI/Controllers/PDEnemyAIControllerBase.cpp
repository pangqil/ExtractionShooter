#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"

#include "Components/StateTreeAIComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/Components/PDPerceptionComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"

DEFINE_LOG_CATEGORY(LogPDAI);

#if ENABLE_DRAW_DEBUG
// AI 디버그 시각화 토글 — PIE 콘솔에서 `pd.ai.debugdraw 1` / `pd.ai.debugdraw 0`.
static TAutoConsoleVariable<int32> CVarPDAIDebugDraw(
	TEXT("pd.ai.debugdraw"),
	0,
	TEXT("AI 시야/이동 디버그 시각화. 0=off, 1=on."),
	ECVF_Cheat
);
#endif

APDEnemyAIControllerBase::APDEnemyAIControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 부모(AAIController)의 PerceptionComponent UPROPERTY 슬롯에 우리 파생 컴포넌트를 주입.
	// 같은 이름의 멤버를 자식 클래스에서 다시 선언하면 UHT shadowing 오류가 나므로
	// 별도 멤버를 두지 않고 부모 멤버를 그대로 사용한다.
	PerceptionComponent = CreateDefaultSubobject<UPDPerceptionComponent>(TEXT("PDPerception"));

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));

	// 디버그 시각화용. CVar 가 0 일 때 본체는 즉시 return 이라 비용 무시 가능.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

UPDPerceptionComponent* APDEnemyAIControllerBase::GetPDPerception() const
{
	return Cast<UPDPerceptionComponent>(PerceptionComponent);
}

void APDEnemyAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 디버그: 진입 체인 1단계 — Possess 자체가 발생했는지, 핵심 컴포넌트가 살아있는지.
	UE_LOG(LogPDAI, Log, TEXT("[%s] OnPossess: Pawn=%s, Perception=%s, StateTreeAI=%s"),
		*GetNameSafe(this),
		*GetNameSafe(InPawn),
		GetPDPerception() ? TEXT("OK") : TEXT("MISSING"),
		StateTreeAIComponent ? TEXT("OK") : TEXT("MISSING"));

	// Senior: Perception 이벤트 구독은 OnPossess 시점에서. Controller 가 미possess 인 동안
	//         감지된 자극은 의미가 없으므로.
	if (UPDPerceptionComponent* PDPerception = GetPDPerception())
	{
		PDPerception->OnTargetSpotted.AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetSpotted);
		PDPerception->OnTargetLost.AddDynamic(this, &APDEnemyAIControllerBase::HandleTargetLost);
		PDPerception->OnNoiseHeard.AddDynamic(this, &APDEnemyAIControllerBase::HandleNoiseHeard);
	}
	else
	{
		UE_LOG(LogPDAI, Warning, TEXT("[%s] OnPossess: PDPerception is null — perception 이벤트가 흐르지 않음."),
			*GetNameSafe(this));
	}

	// StateTree 시작은 StateTreeAIComponent 가 자동 처리(StartLogic) 또는 디자이너가 BP 에서 호출.
	// 여기서는 자동 시작 시도 — bStartLogicAutomatically 가 false 인 경우를 대비해 명시적 호출.
	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StartLogic();
		UE_LOG(LogPDAI, Log, TEXT("[%s] StateTreeAI->StartLogic() 호출됨."), *GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogPDAI, Warning, TEXT("[%s] StateTreeAIComponent is null — StateTree 가 실행되지 않음."),
			*GetNameSafe(this));
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
	// 디버그: 진입 체인 2단계 — 시각 자극이 도달했는지.
	UE_LOG(LogPDAI, Log, TEXT("[%s] HandleTargetSpotted: Target=%s"),
		*GetNameSafe(this), *GetNameSafe(Target));

	// Senior: AIController 가 perception → CombatComponent 결합 책임을 진다.
	//         이렇게 해야 StateTree 는 단순히 HasValidTarget 만 보고 분기 가능 — perception 이벤트
	//         자체를 StateTree 에서 구독하지 않아 그래프가 단순해짐.
	bool bWasNoTarget = false;
	if (APDBipedEnemy* Biped = Cast<APDBipedEnemy>(GetPawn()))
	{
		if (UPDCombatComponent* Combat = Biped->GetCombatComponent())
		{
			bWasNoTarget = !Combat->HasValidTarget();
			Combat->SetCurrentTarget(Target);
		}
	}

	// Senior: StateTree 의 Selector 는 자식이 Running 인 동안 재평가하지 않음.
	//         Idle 의 task 들이 영원히 Running 이라 자체 종료를 못 하므로,
	//         "처음 타겟이 잡힌 시점" 에 한해 트리 재시작으로 Selector 재평가 강제.
	//         이미 타겟이 있던 (Chase/Combat 진행 중) 상황에서는 재시작하지 않아 진행 중 state 보존.
	if (bWasNoTarget && StateTreeAIComponent && StateTreeAIComponent->IsRunning())
	{
		UE_LOG(LogPDAI, Log, TEXT("[%s] StateTree restart — TargetAcquired."), *GetNameSafe(this));
		StateTreeAIComponent->StopLogic(TEXT("TargetAcquired"));
		StateTreeAIComponent->StartLogic();
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

void APDEnemyAIControllerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if ENABLE_DRAW_DEBUG
	if (CVarPDAIDebugDraw.GetValueOnGameThread() != 0)
	{
		DrawAIDebug();
	}
#endif
}

void APDEnemyAIControllerBase::DrawAIDebug() const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	APawn* OwnerPawn = GetPawn();
	if (!World || !OwnerPawn)
	{
		return;
	}

	const FVector Origin = OwnerPawn->GetActorLocation();
	const FVector Forward = OwnerPawn->GetActorForwardVector();

	// 색상 규약: 시야=Green, LoseSight=Yellow, 청각=Cyan, 타겟=Red, NoiseHint=Yellow, Path=Magenta.

	// 1) 시야 콘 — Sight / LoseSight (반각 = PeripheralVisionAngleDegrees).
	if (const UPDPerceptionComponent* Perception = GetPDPerception())
	{
		const float SightRadius = Perception->GetSightRadius();
		const float LoseSightRadius = FMath::Max(Perception->GetLoseSightRadius(), SightRadius);
		const float HalfAngleRad = FMath::DegreesToRadians(Perception->GetPeripheralVisionAngleDegrees());

		DrawDebugCone(World, Origin, Forward, SightRadius,
			HalfAngleRad, HalfAngleRad,
			24, FColor::Green, false, -1.f, SDPG_World, 1.f);

		DrawDebugCone(World, Origin, Forward, LoseSightRadius,
			HalfAngleRad, HalfAngleRad,
			24, FColor(255, 200, 0), false, -1.f, SDPG_World, 0.5f);

		// 2) 청각 반경 — 수평 원 (XY 평면).
		const float HearingRange = Perception->GetHearingRange();
		DrawDebugCircle(World, Origin, HearingRange, 48, FColor::Cyan,
			false, -1.f, SDPG_World, 1.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f),
			/*bDrawAxis=*/false);
	}

	// 3) 현재 타겟 → 빨간 선 + 라벨.
	UPDCombatComponent* Combat = nullptr;
	if (APDBipedEnemy* Biped = Cast<APDBipedEnemy>(OwnerPawn))
	{
		Combat = Biped->GetCombatComponent();
	}
	if (Combat)
	{
		if (AActor* Target = Combat->GetCurrentTarget())
		{
			const FVector TargetLoc = Target->GetActorLocation();
			DrawDebugLine(World, Origin, TargetLoc, FColor::Red, false, -1.f, SDPG_World, 2.f);
			DrawDebugSphere(World, TargetLoc, 40.f, 12, FColor::Red, false, -1.f, SDPG_World, 1.5f);
			DrawDebugString(World, TargetLoc + FVector(0, 0, 120),
				FString::Printf(TEXT("Target: %s"), *GetNameSafe(Target)),
				nullptr, FColor::Red, 0.f, /*bDrawShadow=*/true);
		}

		if (Combat->HasNoiseHint())
		{
			const FVector NoiseLoc = Combat->GetLastNoiseLocation();
			DrawDebugSphere(World, NoiseLoc, 60.f, 16, FColor::Yellow, false, -1.f, SDPG_World, 2.f);
			DrawDebugLine(World, Origin, NoiseLoc, FColor::Yellow, false, -1.f, SDPG_World, 1.f);
			DrawDebugString(World, NoiseLoc + FVector(0, 0, 100),
				TEXT("NoiseHint"), nullptr, FColor::Yellow, 0.f, true);
		}
	}

	// 4) 현재 NavPath — magenta 라인.
	if (const UPathFollowingComponent* PF = GetPathFollowingComponent())
	{
		const FNavPathSharedPtr Path = PF->GetPath();
		if (Path.IsValid() && Path->IsValid())
		{
			const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
			constexpr float ZLift = 10.f; // ground 살짝 띄워서 가시성 확보.
			for (int32 i = 1; i < Points.Num(); ++i)
			{
				DrawDebugLine(World,
					Points[i - 1].Location + FVector(0, 0, ZLift),
					Points[i].Location + FVector(0, 0, ZLift),
					FColor::Magenta, false, -1.f, SDPG_World, 3.f);
			}

			// 최종 goal 위치 표시.
			if (Points.Num() > 0)
			{
				const FVector Goal = Points.Last().Location;
				DrawDebugSphere(World, Goal, 30.f, 12, FColor::Magenta, false, -1.f, SDPG_World, 1.5f);
				DrawDebugString(World, Goal + FVector(0, 0, 80),
					TEXT("Goal"), nullptr, FColor::Magenta, 0.f, true);
			}
		}
	}

	// 5) AI 식별 라벨 (Pawn 머리 위).
	DrawDebugString(World, Origin + FVector(0, 0, 200),
		FString::Printf(TEXT("[%s]"), *GetNameSafe(OwnerPawn)),
		nullptr, FColor::White, 0.f, true);
#endif // ENABLE_DRAW_DEBUG
}

void APDEnemyAIControllerBase::HandleNoiseHeard(AActor* NoiseInstigator, FVector Location)
{
	// Senior: 청각 → CombatComponent.NoiseHint 결합도 시각과 동일하게 AIController 가 담당.
	//         이렇게 해야 StateTree 는 HasNoiseHint / HasValidTarget 두 도메인 신호만 보고 분기 가능.
	// Mid:    이미 시야 타겟이 있다면 청각 hint 는 무시 — 시각 우선순위 유지.
	bool bSuppressedByVisualTarget = false;
	bool bWasNoHint = false;
	if (APDBipedEnemy* Biped = Cast<APDBipedEnemy>(GetPawn()))
	{
		if (UPDCombatComponent* Combat = Biped->GetCombatComponent())
		{
			if (!Combat->HasValidTarget())
			{
				bWasNoHint = !Combat->HasNoiseHint();
				Combat->SetLastNoiseLocation(NoiseInstigator, Location);
			}
			else
			{
				bSuppressedByVisualTarget = true;
			}
		}
	}

	// 디버그: 진입 체인 2단계 — 청각 자극이 들어왔는지, 시각 우선순위로 무시됐는지.
	UE_LOG(LogPDAI, Log, TEXT("[%s] HandleNoiseHeard: Instigator=%s, Loc=%s, Suppressed(VisualTarget)=%s"),
		*GetNameSafe(this),
		*GetNameSafe(NoiseInstigator),
		*Location.ToString(),
		bSuppressedByVisualTarget ? TEXT("true") : TEXT("false"));

	// "처음 hint 가 잡힌 시점" 에 한해 트리 재시작 — Idle 에 갇혀있을 때 Investigate 로 진입 강제.
	if (bWasNoHint && StateTreeAIComponent && StateTreeAIComponent->IsRunning())
	{
		UE_LOG(LogPDAI, Log, TEXT("[%s] StateTree restart — NoiseHintAcquired."), *GetNameSafe(this));
		StateTreeAIComponent->StopLogic(TEXT("NoiseHintAcquired"));
		StateTreeAIComponent->StartLogic();
	}

	OnNoiseHeard(NoiseInstigator, Location);
}
