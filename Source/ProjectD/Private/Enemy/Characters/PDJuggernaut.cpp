#include "Enemy/Characters/PDJuggernaut.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/PDDamageable.h"
#include "TimerManager.h"

APDJuggernaut::APDJuggernaut()
{
	TeamID = 2; // Hostile.

	// 거치 무기는 메시만 부착 — 발사/타이밍/데미지는 본 클래스가 직접 제어(무기 프레임워크 미사용).
	MachineGunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MachineGunMesh"));
	MachineGunMesh->SetupAttachment(GetMesh(), MachineGunSocketName);
	MachineGunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MissileLauncherMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MissileLauncherMesh"));
	MissileLauncherMesh->SetupAttachment(GetMesh(), MissilePortSocketName);
	MissileLauncherMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDJuggernaut::BeginPlay()
{
	Super::BeginPlay();

	// 디자이너가 소켓 이름을 변경했을 수 있으니 현재 값으로 재부착(ctor 부착은 C++ 디폴트 기준).
	if (USkeletalMeshComponent* Body = GetMesh())
	{
		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
		if (MachineGunMesh)      MachineGunMesh->AttachToComponent(Body, Rules, MachineGunSocketName);
		if (MissileLauncherMesh) MissileLauncherMesh->AttachToComponent(Body, Rules, MissilePortSocketName);
	}

	// 활성화 범위 판정은 서버에서만. 폰 BeginPlay 시점에 컨트롤러가 아직 없을 수 있으므로
	// 루프 타이머로 돌리고 매 tick 컨트롤러 유효성을 확인.
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ActivationTimerHandle, this,
			&APDJuggernaut::TickActivationCheck, ActivationCheckInterval, /*bLoop=*/true);
	}
}

void APDJuggernaut::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 패턴(조준+발사) 동안 플레이어를 지속 추적 — BT/focus 와 무관하게 C++가 직접 yaw 회전.
	if (bIsExecutingPattern)
	{
		FacePatternTarget(DeltaSeconds);

		// 패턴2: 예약된 미사일 착탄을 world-time 으로 처리.
		if (CurrentPattern == EPDJuggernautPattern::Annihilate)
		{
			TickPattern2Impacts();
		}
	}
}

void APDJuggernaut::FacePatternTarget(float DeltaSeconds)
{
	const AActor* Target = ActiveTarget.Get();
	if (!Target) return;

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return;

	const FRotator CurrentRot = GetActorRotation();
	const FRotator DesiredRot(CurrentRot.Pitch, ToTarget.Rotation().Yaw, CurrentRot.Roll);
	const FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, DesiredRot, DeltaSeconds, PatternTurnRateDegPerSec);

	// 액터 + ControlRotation 동시 갱신 — CharacterMovement 회전 정책(bUseControllerDesiredRotation 등)과 충돌 방지.
	SetActorRotation(NewRot);
	if (APDEnemyAIControllerBase* AICon = GetEnemyController())
	{
		AICon->SetControlRotation(FRotator(0.f, NewRot.Yaw, 0.f));
	}
}

void APDJuggernaut::OnEnterState_Dead()
{
	// 모든 보스 타이머 정리 후 부모 사망 처리(발사 timer 정지·무기·LootBox 드랍).
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(ActivationTimerHandle);
		TM.ClearTimer(DeactivationTimerHandle);
		TM.ClearTimer(PatternCooldownHandle);
		TM.ClearTimer(PatternChargeHandle);
		TM.ClearTimer(Pattern1FireHandle);
		TM.ClearTimer(Pattern2LaunchHandle);
	}

	Pattern2PendingImpacts.Reset();
	bIsExecutingPattern = false;
	CurrentPattern = EPDJuggernautPattern::None;

	Super::OnEnterState_Dead();
}

// ───────────────────────────── 활성화 생애주기 ─────────────────────────────

void APDJuggernaut::TickActivationCheck()
{
	if (!HasAuthority() || IsDead()) return;

	switch (Phase)
	{
	case EPDJuggernautPhase::Dormant:
	{
		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			Activate(Player);
		}
		break;
	}
	case EPDJuggernautPhase::Returning:
	{
		// 복귀 중 재진입 시 즉시 재활성, 아니면 원위치 도착 판정.
		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			Activate(Player);
		}
		else
		{
			CheckReturnHomeArrival();
		}
		break;
	}
	case EPDJuggernautPhase::Active:
	{
		// 패턴 진행 중에는 거리 무관 — 비활성화 보류(요구사항).
		if (bIsExecutingPattern) break;

		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			// 타겟 신선도 유지(가장 가까운 플레이어로 갱신).
			if (Player != ActiveTarget.Get())
			{
				ActiveTarget = Player;
				SetCombatTarget(Player);
			}
		}
		else
		{
			BeginDeactivation();
		}
		break;
	}
	case EPDJuggernautPhase::Deactivating:
	{
		// 카운트다운 중 재진입 → 즉시 재활성(타이머 취소는 Activate 가 수행).
		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			Activate(Player);
		}
		break;
	}
	}
}

void APDJuggernaut::EnterPhase(EPDJuggernautPhase NewPhase)
{
	Phase = NewPhase;

	switch (NewPhase)
	{
	case EPDJuggernautPhase::Active:
		SetEnemyState(EPDEnemyState::Combat);
		break;

	case EPDJuggernautPhase::Deactivating:
		// 그 자리에 정지(요구사항). BT 는 BossPhase 로 Strafe/사격 분기를 닫음.
		SetEnemyState(EPDEnemyState::Alert);
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		break;

	case EPDJuggernautPhase::Returning:
	case EPDJuggernautPhase::Dormant:
		SetEnemyState(EPDEnemyState::Idle);
		break;
	}

	SyncStateToBlackboard();
}

void APDJuggernaut::Activate(AActor* Target)
{
	if (!Target) return;

	// 진행 중이던 비활성화 카운트다운 취소.
	GetWorldTimerManager().ClearTimer(DeactivationTimerHandle);

	const bool bWasActive = (Phase == EPDJuggernautPhase::Active);

	ActiveTarget = Target;
	EnterPhase(EPDJuggernautPhase::Active);
	SetCombatTarget(Target);

	// 이미 Active 였으면(타겟 갱신 경로) 패턴 스케줄을 다시 깔지 않음.
	if (!bWasActive)
	{
		ScheduleNextPattern();
		BP_OnActivated(Target);
	}
}

void APDJuggernaut::BeginDeactivation()
{
	EnterPhase(EPDJuggernautPhase::Deactivating);

	// 비활성화 중에는 일반공격·패턴 모두 중단.
	StopContinuousFire();
	GetWorldTimerManager().ClearTimer(PatternCooldownHandle);

	BP_OnBeginDeactivation();

	GetWorldTimerManager().SetTimer(DeactivationTimerHandle, this,
		&APDJuggernaut::CompleteDeactivation, DeactivationDelay, /*bLoop=*/false);
}

void APDJuggernaut::CompleteDeactivation()
{
	FullHeal();

	ActiveTarget = nullptr;
	SetCombatTarget(nullptr);   // 타겟 해제 → BB.TargetActor 클리어(AIController 동기화)

	EnterPhase(EPDJuggernautPhase::Returning);
	BP_OnDeactivated();
	// 실제 원위치 이동은 BT(BossPhase==Returning) 가 MoveTo(HomeLocation) 로 수행.
}

void APDJuggernaut::CheckReturnHomeArrival()
{
	const APDEnemyAIControllerBase* AICon = GetEnemyController();
	const UBlackboardComponent* BB = AICon ? AICon->GetBlackboardComponent() : nullptr;
	if (!BB) return;

	const FVector Home = BB->GetValueAsVector(PDBTKeys::HomeLocation);
	if (Home.ContainsNaN() || !FMath::IsFinite(Home.X)) return;

	if (FVector::DistSquared(GetActorLocation(), Home) <= HomeArrivalTolerance * HomeArrivalTolerance)
	{
		EnterPhase(EPDJuggernautPhase::Dormant);
		BP_OnReturnHomeComplete();
	}
}

AActor* APDJuggernaut::FindNearestPlayerInRange(float Range) const
{
	const UWorld* World = GetWorld();
	if (!World) return nullptr;

	const FVector SelfLoc = GetActorLocation();
	const float RangeSq = Range * Range;

	AActor* Closest = nullptr;
	float ClosestSq = TNumericLimits<float>::Max();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (!PlayerPawn || PlayerPawn == this) continue;

		// 사망/다운된 플레이어는 타겟에서 제외.
		if (PlayerPawn->Implements<UPDDamageable>() && !IPDDamageable::Execute_IsAlive(PlayerPawn)) continue;

		const float DistSq = FVector::DistSquared(SelfLoc, PlayerPawn->GetActorLocation());
		if (DistSq <= RangeSq && DistSq < ClosestSq)
		{
			ClosestSq = DistSq;
			Closest = PlayerPawn;
		}
	}
	return Closest;
}

void APDJuggernaut::FullHeal()
{
	if (!HasAuthority() || !AttributeSet) return;

	// InitializeAttributes 와 동일 패턴 — 전 부위 HP 를 Max 로 복원.
	AttributeSet->SetHeadHP (AttributeSet->GetMaxHeadHP());
	AttributeSet->SetTorsoHP(AttributeSet->GetMaxTorsoHP());
	AttributeSet->SetArmLHP (AttributeSet->GetMaxArmLHP());
	AttributeSet->SetArmRHP (AttributeSet->GetMaxArmRHP());
	AttributeSet->SetLegLHP (AttributeSet->GetMaxLegLHP());
	AttributeSet->SetLegRHP (AttributeSet->GetMaxLegRHP());
}

// ───────────────────────────────── 패턴 ─────────────────────────────────

void APDJuggernaut::ScheduleNextPattern()
{
	if (Phase != EPDJuggernautPhase::Active) return;

	const float Cooldown = FMath::FRandRange(
		FMath::Min(PatternCooldownMin, PatternCooldownMax),
		FMath::Max(PatternCooldownMin, PatternCooldownMax));

	GetWorldTimerManager().SetTimer(PatternCooldownHandle, this,
		&APDJuggernaut::TryStartPattern, FMath::Max(Cooldown, 0.1f), /*bLoop=*/false);
}

void APDJuggernaut::TryStartPattern()
{
	if (Phase != EPDJuggernautPhase::Active || bIsExecutingPattern) return;

	AActor* Target = ActiveTarget.Get();
	if (!Target)
	{
		ScheduleNextPattern();
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	switch (SelectPatternByDistance(Dist))
	{
	case EPDJuggernautPattern::Pulverize:
		StartPattern1(Target);
		break;

	case EPDJuggernautPattern::Annihilate:
		StartPattern2(Target);
		break;

	default:
		// 조건에 맞는 (구현된) 패턴 없음 — 일반공격 유지하며 다음 쿨다운 재시도.
		// TODO: Pattern3(Extinction/추적 미사일) 구현 후 분기 추가.
		ScheduleNextPattern();
		break;
	}
}

EPDJuggernautPattern APDJuggernaut::SelectPatternByDistance(float DistToTarget) const
{
	// 거리 1200 이하 → 패턴1(분쇄/머신건), 초과 → 패턴2(섬멸/미사일 스웜).
	if (DistToTarget <= Pattern1MaxRange) return EPDJuggernautPattern::Pulverize;
	return EPDJuggernautPattern::Annihilate;
}

void APDJuggernaut::StartPattern1(AActor* Target)
{
	bIsExecutingPattern = true;
	CurrentPattern = EPDJuggernautPattern::Pulverize;
	SyncStateToBlackboard();

	// 패턴 중 일반공격 중단(요구사항). BT 는 bIsExecutingPattern 로 사격/Strafe 분기를 닫음.
	// 타겟 향한 회전은 Tick(FacePatternTarget)이 담당 — 조준+발사 내내 지속.
	StopContinuousFire();

	BP_OnPattern1BeginAim(Target);

	GetWorldTimerManager().SetTimer(PatternChargeHandle, this,
		&APDJuggernaut::Pattern1BeginFire, FMath::Max(Pattern1ChargeTime, 0.01f), /*bLoop=*/false);
}

void APDJuggernaut::Pattern1BeginFire()
{
	const AActor* Target = ActiveTarget.Get();
	if (!Target)
	{
		EndPattern();
		return;
	}

	// 초기 조준 방향(BP 발사 연출용). 이후 매 발사 틱에서 타겟을 추적해 갱신(Track).
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation());
	Pattern1AimDir = ToTarget.GetSafeNormal2D();
	if (Pattern1AimDir.IsNearlyZero()) Pattern1AimDir = GetActorForwardVector().GetSafeNormal2D();

	Pattern1FireElapsed = 0.f;
	Pattern1MuzzleIndex = 0;

	BP_OnPattern1BeginFire(Pattern1AimDir);

	GetWorldTimerManager().SetTimer(Pattern1FireHandle, this,
		&APDJuggernaut::Pattern1FireTick, FMath::Max(Pattern1FireInterval, 0.02f), /*bLoop=*/true);
}

void APDJuggernaut::Pattern1FireTick()
{
	Pattern1FireElapsed += FMath::Max(Pattern1FireInterval, 0.02f);

	// 지속 추적(Track): 매 틱 현재 타겟 방향으로 조준 방향 갱신. 몸체 회전은 Tick(FacePatternTarget)이 담당.
	if (const AActor* Target = ActiveTarget.Get())
	{
		const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero()) Pattern1AimDir = ToTarget;
	}

	// 머즐 소켓 번갈아 사용 — VFX 훅 + (LOS 트레이스 시작점).
	FName Socket = NAME_None;
	FVector MuzzleLoc = GetActorLocation();
	if (Pattern1MuzzleSockets.Num() > 0)
	{
		Socket = Pattern1MuzzleSockets[Pattern1MuzzleIndex % Pattern1MuzzleSockets.Num()];
		if (MachineGunMesh && Socket != NAME_None && MachineGunMesh->DoesSocketExist(Socket))
		{
			MuzzleLoc = MachineGunMesh->GetSocketLocation(Socket);
		}
	}
	++Pattern1MuzzleIndex;

	BP_OnPattern1FireShot(Socket, MuzzleLoc);

	ApplyPattern1DamageInCone();

	if (Pattern1FireElapsed >= Pattern1FireDuration)
	{
		EndPattern();
	}
}

void APDJuggernaut::ApplyPattern1DamageInCone()
{
	if (!HasAuthority()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const FVector Origin = GetActorLocation();
	const float RangeSq = Pattern1Range * Pattern1Range;
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(Pattern1ConeHalfAngleDeg));

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (!PlayerPawn || PlayerPawn == this) continue;
		if (!PlayerPawn->Implements<UPDDamageable>()) continue;
		if (!IPDDamageable::Execute_IsAlive(PlayerPawn)) continue;

		const FVector ToPlayer = PlayerPawn->GetActorLocation() - Origin;
		if (ToPlayer.SizeSquared() > RangeSq) continue;

		// 잠긴 조준 방향 기준 부채꼴(2D) 판정.
		const FVector Dir2D = ToPlayer.GetSafeNormal2D();
		if ((Dir2D | Pattern1AimDir) < CosHalfAngle) continue;

		// 벽 너머 관통 방지 — 가시선 확인.
		FHitResult Block;
		FCollisionQueryParams Params(FName(TEXT("JuggernautPattern1LOS")), false, this);
		Params.AddIgnoredActor(PlayerPawn);
		const FVector TraceStart = Origin + FVector(0, 0, 80.f);
		const FVector TraceEnd = PlayerPawn->GetActorLocation();
		if (World->LineTraceSingleByChannel(Block, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			continue; // 사이에 막힘.
		}

		FPDDamageInfo DamageInfo;
		DamageInfo.BaseDamage = Pattern1DamagePerTick;
		DamageInfo.Instigator = this;
		DamageInfo.DamageTypeClass = nullptr;
		IPDDamageable::Execute_ApplyDamage(PlayerPawn, DamageInfo);
	}
}

void APDJuggernaut::EndPattern()
{
	const EPDJuggernautPattern Finished = CurrentPattern;

	FTimerManager& TM = GetWorldTimerManager();
	TM.ClearTimer(PatternChargeHandle);
	TM.ClearTimer(Pattern1FireHandle);
	TM.ClearTimer(Pattern2LaunchHandle);
	Pattern2PendingImpacts.Reset();

	bIsExecutingPattern = false;
	CurrentPattern = EPDJuggernautPattern::None;
	SyncStateToBlackboard();

	// 종료된 패턴에 맞는 BP 정리 훅 호출.
	switch (Finished)
	{
	case EPDJuggernautPattern::Pulverize:  BP_OnPattern1End(); break;
	case EPDJuggernautPattern::Annihilate: BP_OnPattern2End(); break;
	default: break;
	}

	// 활성 상태면 다음 패턴 쿨다운 재예약(일반공격은 BT 가 자동 재개).
	if (Phase == EPDJuggernautPhase::Active)
	{
		ScheduleNextPattern();
	}
}

// ───────────────────────── 패턴2: 섬멸 (미사일 스웜) ─────────────────────────

void APDJuggernaut::StartPattern2(AActor* Target)
{
	bIsExecutingPattern = true;
	CurrentPattern = EPDJuggernautPattern::Annihilate;
	SyncStateToBlackboard();

	// 패턴 중 일반공격 중단. 회전(FacePatternTarget)은 Tick 이 담당.
	StopContinuousFire();

	Pattern2LaunchedCount = 0;
	Pattern2HatchIndex = 0;
	Pattern2PendingImpacts.Reset();

	BP_OnPattern2Begin(Target);

	// 첫 발 즉시 + 이후 stagger 발사.
	Pattern2LaunchTick();
	if (Pattern2LaunchedCount < Pattern2MissileCount)
	{
		GetWorldTimerManager().SetTimer(Pattern2LaunchHandle, this,
			&APDJuggernaut::Pattern2LaunchTick, FMath::Max(Pattern2LaunchInterval, 0.02f), /*bLoop=*/true);
	}
}

void APDJuggernaut::Pattern2LaunchTick()
{
	// 착탄 중심 = 발사 시점의 플레이어 위치(스웜이 이동을 어느 정도 따라감). 타겟 무효 시 전방 폴백.
	const AActor* Target = ActiveTarget.Get();
	const FVector Center = Target
		? Target->GetActorLocation()
		: GetActorLocation() + GetActorForwardVector() * (Pattern1MaxRange + Pattern2ScatterRadius);

	// 플레이어 주변 디스크 내 균등 랜덤 지점.
	const float Angle  = FMath::DegreesToRadians(FMath::FRandRange(0.f, 360.f));
	const float Radius = Pattern2ScatterRadius * FMath::Sqrt(FMath::FRand());
	const FVector Impact = Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);

	// 발사 해치 소켓 순환.
	FName Hatch = NAME_None;
	FVector LaunchLoc = GetActorLocation();
	if (Pattern2HatchSockets.Num() > 0)
	{
		Hatch = Pattern2HatchSockets[Pattern2HatchIndex % Pattern2HatchSockets.Num()];
		if (MissileLauncherMesh && Hatch != NAME_None && MissileLauncherMesh->DoesSocketExist(Hatch))
		{
			LaunchLoc = MissileLauncherMesh->GetSocketLocation(Hatch);
		}
	}
	++Pattern2HatchIndex;

	BP_OnPattern2Launch(Hatch, LaunchLoc, Impact, Pattern2TravelTime);

	// 착탄 예약 — Tick 이 world-time 으로 처리(8개 개별 타이머의 수명 이슈 회피).
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Pattern2PendingImpacts.Add({ Impact, Now + Pattern2TravelTime });

	++Pattern2LaunchedCount;
	if (Pattern2LaunchedCount >= Pattern2MissileCount)
	{
		GetWorldTimerManager().ClearTimer(Pattern2LaunchHandle);
	}
}

void APDJuggernaut::TickPattern2Impacts()
{
	const UWorld* World = GetWorld();
	if (!World) return;

	const float Now = World->GetTimeSeconds();

	// 시간 도달분 폭발 처리(역순 제거 안전).
	for (int32 i = Pattern2PendingImpacts.Num() - 1; i >= 0; --i)
	{
		if (Now >= Pattern2PendingImpacts[i].ImpactTime)
		{
			const FVector Loc = Pattern2PendingImpacts[i].Location;
			Pattern2PendingImpacts.RemoveAt(i);
			BP_OnPattern2Impact(Loc);
			ApplyPattern2Damage(Loc);
		}
	}

	// 모두 발사 + 모두 착탄 → 패턴 종료.
	if (Pattern2LaunchedCount >= Pattern2MissileCount && Pattern2PendingImpacts.Num() == 0)
	{
		EndPattern();
	}
}

void APDJuggernaut::ApplyPattern2Damage(const FVector& Center)
{
	if (!HasAuthority()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const float RadiusSq = Pattern2ImpactRadius * Pattern2ImpactRadius;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (!PlayerPawn || PlayerPawn == this) continue;
		if (!PlayerPawn->Implements<UPDDamageable>()) continue;
		if (!IPDDamageable::Execute_IsAlive(PlayerPawn)) continue;

		// 착탄 원(2D) 안의 플레이어만 피해 — 탑뷰 기준.
		if ((PlayerPawn->GetActorLocation() - Center).SizeSquared2D() > RadiusSq) continue;

		FPDDamageInfo DamageInfo;
		DamageInfo.BaseDamage = Pattern2Damage;
		DamageInfo.Instigator = this;
		DamageInfo.DamageTypeClass = nullptr;
		IPDDamageable::Execute_ApplyDamage(PlayerPawn, DamageInfo);
	}
}

// ─────────────────────────── 타겟 / BB 동기화 ───────────────────────────

APDEnemyAIControllerBase* APDJuggernaut::GetEnemyController() const
{
	return Cast<APDEnemyAIControllerBase>(GetController());
}

void APDJuggernaut::SetCombatTarget(AActor* Target)
{
	// CombatComponent 경유 — RequestAttack 의 타겟/사거리 검증 + AIController 가 BB.TargetActor 로 동기화.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		if (Target) Combat->SetCurrentTarget(Target);
		else        Combat->ClearCurrentTarget();
	}
}

void APDJuggernaut::SyncStateToBlackboard()
{
	APDEnemyAIControllerBase* AICon = GetEnemyController();
	UBlackboardComponent* BB = AICon ? AICon->GetBlackboard() : nullptr;
	if (!BB) return;

	BB->SetValueAsEnum(PDBTKeys::BossPhase, static_cast<uint8>(Phase));
	BB->SetValueAsBool(PDBTKeys::bIsExecutingPattern, bIsExecutingPattern);

	// EnemyState 도 동기화 — perception 의 HandleTargetLost 가 BB.EnemyState 로 교전 여부를 판단해
	// 비-교전 상태면 TargetActor 를 클리어하므로, 보스의 CombatComponent 타겟과 충돌하지 않도록 맞춰둠.
	BB->SetValueAsEnum(PDBTKeys::EnemyState, static_cast<uint8>(GetEnemyState()));
}
