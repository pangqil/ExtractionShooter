#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "Enemy/Interfaces/PDBossGimmickInterface.h"
#include "PDJuggernaut.generated.h"

class USkeletalMeshComponent;
class APDEnemyAIControllerBase;

/** Juggernaut 보스 생애주기 단계. BB(BossPhase)로 노출 → BT 분기 게이트. */
UENUM(BlueprintType)
enum class EPDJuggernautPhase : uint8
{
	Dormant      UMETA(DisplayName = "Dormant"),       // 비활성: 원위치 대기
	Active       UMETA(DisplayName = "Active"),         // 활성: Strafe + Sniper + 패턴
	Deactivating UMETA(DisplayName = "Deactivating"),   // 범위 이탈: 정지 후 카운트다운
	Returning    UMETA(DisplayName = "Returning"),      // 풀회복 후 원위치 복귀
};

/** 공격 패턴 식별자. 패턴2·3(미사일)은 후속 단계에서 구현. */
UENUM(BlueprintType)
enum class EPDJuggernautPattern : uint8
{
	None       UMETA(DisplayName = "None"),
	Pulverize  UMETA(DisplayName = "Pulverize (MachineGun)"),    // 패턴1 분쇄
	Annihilate UMETA(DisplayName = "Annihilate (Missile Swarm)"),// 패턴2 섬멸 (미구현)
	Extinction UMETA(DisplayName = "Extinction (Homing Missile)"),// 패턴3 소멸 (미구현)
};

/**
 * Juggernaut 보스.
 *  - APDSoldier 상속: Sniper 일반공격(단발 경로)·LootBox 드랍·부위별 HP 를 그대로 재사용.
 *    → 디자이너 BP: DefaultWeaponClass=Sniper, CombatComponent.AttackRange=2500, LootTable/CorpseContainerClass 설정.
 *  - 본 클래스가 추가하는 것: 활성화 생애주기 + 랜덤 쿨다운 패턴 + 거치무기.
 *
 * 역할 분담:
 *  - C++  : 활성/비활성/풀회복/복귀 상태머신, 패턴 디스패처·타이밍·데미지. 타겟은 CombatComponent 로 지정
 *           (RequestAttack 검증 + AIController 가 BB.TargetActor 로 자동 동기화).
 *  - BT   : BossPhase / bIsExecutingPattern / TargetActor 를 읽어 Strafe·FireAtTarget(Sniper)·MoveTo(Home) 분기.
 *  - BP   : AoE 인디케이터·머즐 VFX·몽타주 등 연출 (BP_On* 훅).
 *
 * 활성화 규칙(요구사항):
 *  - ActivationRange(3000) 안에 플레이어 진입 → 활성(타겟 지정).
 *  - 범위 이탈 → 그 자리 정지 후 DeactivationDelay(5초) → 비활성(타겟 해제) + 풀회복 + 원위치 복귀.
 *  - 카운트다운 중 재진입 → 즉시 재활성.
 *  - 패턴 시작 후에는 거리 조건을 벗어나도 중단하지 않음(비활성화 보류).
 */
UCLASS(Blueprintable)
class PROJECTD_API APDJuggernaut : public APDSoldier, public IPDBossGimmickInterface
{
	GENERATED_BODY()

public:
	APDJuggernaut();

	UFUNCTION(BlueprintPure, Category = "PD|Boss")
	FORCEINLINE EPDJuggernautPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "PD|Boss")
	FORCEINLINE bool IsBossActive() const { return Phase == EPDJuggernautPhase::Active; }

	UFUNCTION(BlueprintPure, Category = "PD|Boss")
	FORCEINLINE bool IsExecutingPattern() const { return bIsExecutingPattern; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnEnterState_Dead() override;

	// ─── 거치 무기 (메시만 부착, 발사·타이밍은 본 클래스가 직접 제어) ─────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Boss|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MachineGunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Boss|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MissileLauncherMesh;

	/** 본체 메시에서 머신건 메시를 부착할 소켓. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Weapon")
	FName MachineGunSocketName = TEXT("MachineGunSocket");

	/** 본체 메시에서 미사일포 메시를 부착할 소켓. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Weapon")
	FName MissilePortSocketName = TEXT("MissilePortSocket");

	// ─── 활성화 ───────────────────────────────────────────────────────────────
	/** 이 거리 안에 플레이어가 들어오면 활성화. 이탈 판정도 동일 거리. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.0"))
	float ActivationRange = 3000.f;

	/** 범위 이탈 후 비활성화까지 정지 대기 시간(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.0"))
	float DeactivationDelay = 5.f;

	/** 활성화 범위 판정 주기(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.05"))
	float ActivationCheckInterval = 0.2f;

	/** 원위치 도착 판정 허용 반경(cm). 이 안에 들어오면 Dormant 로 전환. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.0"))
	float HomeArrivalTolerance = 250.f;

	// ─── 패턴 스케줄 ──────────────────────────────────────────────────────────
	/** 다음 패턴까지 쿨다운 랜덤 범위(초) 하한. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern", meta = (ClampMin = "0.0"))
	float PatternCooldownMin = 8.f;

	/** 다음 패턴까지 쿨다운 랜덤 범위(초) 상한. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern", meta = (ClampMin = "0.0"))
	float PatternCooldownMax = 14.f;

	// ─── 패턴1: 분쇄 (머신건) ─────────────────────────────────────────────────
	/** 패턴1 발동 조건: 타겟 거리 ≤ 이 값. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1MaxRange = 1200.f;

	/** 조준·차징 시간(초). 이 동안 인디케이터가 플레이어를 추적(연출은 BP). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1ChargeTime = 5.f;

	/** 발사 지속 시간(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1FireDuration = 8.f;

	/** 발사 틱 간격(초) — 데미지 판정 + 머즐 VFX 훅 빈도. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.02"))
	float Pattern1FireInterval = 0.1f;

	/** 발사 사거리(cm). 활성/비활성 거리(3000)와 동일 — 패턴 시작 후 거리 무관 지속. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1Range = 3000.f;

	/** AoE 반각(deg). 잠긴 조준 방향 기준 좌우 허용각 — 이 부채꼴 안의 플레이어만 피격. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float Pattern1ConeHalfAngleDeg = 12.f;

	/** 발사 틱당 데미지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1DamagePerTick = 6.f;

	/** 머신건 머즐 소켓들(MachineGunMesh 기준). 발사 틱마다 번갈아 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TArray<FName> Pattern1MuzzleSockets = { TEXT("MGMuzzle1"), TEXT("MGMuzzle2") };

	/** 패턴(조준+발사) 중 플레이어를 바라보는 회전 속도(deg/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern", meta = (ClampMin = "1.0"))
	float PatternTurnRateDegPerSec = 180.f;

	// ─── 패턴2: 섬멸 (미사일 스웜) ────────────────────────────────────────────
	/** 발사할 미사일 수. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "1"))
	int32 Pattern2MissileCount = 8;

	/** 발사 해치 소켓들(MissileLauncherMesh 기준). 발사마다 순환 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TArray<FName> Pattern2HatchSockets = {
		TEXT("Missilehatch0"), TEXT("Missilehatch1"), TEXT("Missilehatch2"), TEXT("Missilehatch3"),
		TEXT("Missilehatch4"), TEXT("Missilehatch5"), TEXT("Missilehatch6"), TEXT("Missilehatch7") };

	/** 미사일 간 발사 간격(초) — 스웜 순차 발사 stagger. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2LaunchInterval = 0.15f;

	/** 발사 → 착탄 시간(초). 인디케이터 차오름 시간 = 미사일 비행 시간(BP 연출과 동기). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.1"))
	float Pattern2TravelTime = 2.5f;

	/** 착탄 지점이 플레이어 주변으로 흩어지는 반경(cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2ScatterRadius = 600.f;

	/** 착탄 AoE 반경(cm). 이 안의 플레이어가 피해. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2ImpactRadius = 250.f;

	/** 착탄 1회당 데미지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2Damage = 25.f;

	// ─── BP 연출 훅 (타이밍·데미지는 C++, 비주얼만 BP) ──────────────────────────
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnActivated(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnBeginDeactivation();

	/** 비활성화 확정(풀회복 직후). 복귀 시작 시점. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnDeactivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnReturnHomeComplete();

	/** 패턴1 조준 시작 — 빨간 AoE 인디케이터 표시 + 차징(ChargeTime). Target 을 향해 표시. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1BeginAim(AActor* Target);

	/** 패턴1 발사 시작 — 조준 방향 잠금. 머신건 루프 VFX/사운드 시작, 인디케이터 고정. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1BeginFire(FVector AimDirection);

	/** 발사 틱마다 — 머즐 플래시/트레이서 등. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1FireShot(FName MuzzleSocket, FVector MuzzleLocation);

	/** 패턴1 종료 — 인디케이터 숨김, 머신건 VFX 정지. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1End();

	/** 패턴2 시작(스웜 발사 직전 windup 연출용). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2Begin(AActor* Target);

	/** 미사일 1발 발사 — 해치에서 cosmetic 미사일 스폰 + 착탄 지점에 인디케이터(TravelTime 동안 차오름). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2Launch(FName HatchSocket, FVector LaunchLocation, FVector ImpactLocation, float TravelTime);

	/** 착탄 — 폭발 VFX/사운드. 데미지는 C++가 적용. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2Impact(FVector ImpactLocation);

	/** 패턴2 종료(모든 미사일 착탄 완료). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2End();

private:
	// ─── 활성화 생애주기 ───
	void TickActivationCheck();
	void EnterPhase(EPDJuggernautPhase NewPhase);
	void Activate(AActor* Target);
	void BeginDeactivation();
	void CompleteDeactivation();      // 풀회복 + Returning 전환
	void CheckReturnHomeArrival();
	AActor* FindNearestPlayerInRange(float Range) const;
	void FullHeal();                  // 전 부위 HP = Max

	// ─── 패턴 ───
	void ScheduleNextPattern();
	void TryStartPattern();
	EPDJuggernautPattern SelectPatternByDistance(float DistToTarget) const;
	void StartPattern1(AActor* Target);
	void Pattern1BeginFire();
	void Pattern1FireTick();
	void EndPattern();
	void ApplyPattern1DamageInCone();

	void StartPattern2(AActor* Target);
	void Pattern2LaunchTick();          // 미사일 1발 발사 + 착탄 예약
	void TickPattern2Impacts();         // world-time 기준 착탄 처리(Tick에서 호출)
	void ApplyPattern2Damage(const FVector& Center);

	// 패턴 중 플레이어를 향해 yaw 회전(BT/focus 무관, 매 프레임).
	void FacePatternTarget(float DeltaSeconds);

	// ─── 타겟/BB 동기화 ───
	APDEnemyAIControllerBase* GetEnemyController() const;
	void SetCombatTarget(AActor* Target);   // CombatComponent 경유 → RequestAttack + BB 자동 동기화
	void SyncStateToBlackboard();           // BossPhase + bIsExecutingPattern

	EPDJuggernautPhase Phase = EPDJuggernautPhase::Dormant;
	bool bIsExecutingPattern = false;
	EPDJuggernautPattern CurrentPattern = EPDJuggernautPattern::None;

	TWeakObjectPtr<AActor> ActiveTarget;

	// 패턴1 발사 단계에서 잠긴 조준 방향(월드, 정규화 2D) + 누적 시간 + 머즐 토글 인덱스.
	FVector Pattern1AimDir = FVector::ForwardVector;
	float   Pattern1FireElapsed = 0.f;
	int32   Pattern1MuzzleIndex = 0;

	// 패턴2: 예약된 착탄(위치 + world-time). Tick 이 시간 도달분을 폭발 처리.
	struct FPDPendingImpact
	{
		FVector Location = FVector::ZeroVector;
		float   ImpactTime = 0.f;
	};
	TArray<FPDPendingImpact> Pattern2PendingImpacts;
	int32 Pattern2LaunchedCount = 0;
	int32 Pattern2HatchIndex = 0;

	FTimerHandle ActivationTimerHandle;
	FTimerHandle DeactivationTimerHandle;
	FTimerHandle PatternCooldownHandle;
	FTimerHandle PatternChargeHandle;    // 조준 → 발사 전환
	FTimerHandle Pattern1FireHandle;     // 발사 루프
	FTimerHandle Pattern2LaunchHandle;   // 미사일 스웜 stagger 발사
};
