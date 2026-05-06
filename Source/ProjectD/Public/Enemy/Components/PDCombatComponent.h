#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDCombatComponent.generated.h"

class AActor;
class APDEnemyAIControllerBase;

/** 타겟이 (재)지정되었을 때. NewTarget이 nullptr이면 타겟 해제. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnTargetChanged, AActor*, NewTarget);
/** Combat 컴포넌트가 공격을 요청했을 때. 실제 발사/모션은 BP / 무기 컴포넌트에서 처리. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnAttackRequested, AActor*, Target);

/**
 * Enemy 공격 행동 컴포넌트.
 *  - 타겟 추적 (TWeakObjectPtr)
 *  - 공격 쿨다운 관리
 *  - "공격하라" 라는 의도만 broadcast — 실제 데미지/이펙트/애니는 디자이너 BP나
 *    APDWeaponBase 측에서 처리.
 *  - 주변 동료에게 타겟 전파 (NotifyAlliesInRadius).
 *
 * Senior 관점: 무기/데미지 시스템과 직접 결합하지 않고 delegate로만 통보 → SRP 준수,
 *              테스트/모킹 용이. 추후 GAS Ability로 교체 시 본 컴포넌트는 디스패처 역할만.
 *
 * Mid 관점: TWeakObjectPtr로 타겟 보유 → 타겟 destroy 시 GC 안전. 쿨다운은 GetWorld()->GetTimeSeconds() 기반으로
 *           타이머 핸들 없이 단순 비교 (메모리 절약, 일시정지 영향 받음 — 의도적 동작).
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDCombatComponent();

	// ---------------------- Target ----------------------

	UFUNCTION(BlueprintCallable, Category = "PD|Combat")
	void SetCurrentTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "PD|Combat")
	void ClearCurrentTarget();

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	bool HasValidTarget() const;

	// ---------------------- Attack ----------------------

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	bool CanAttack() const;

	/**
	 * 공격 시도. 쿨다운/타겟 검증 통과 시 OnAttackRequested broadcast 후 true.
	 * 실제 데미지/이펙트/애니메이션은 구독 측에서 처리.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Combat")
	bool RequestAttack();

	/** 공격 쿨다운 중인지 */
	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	bool IsOnCooldown() const;

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	float GetCooldownRemaining() const;

	// ---------------------- Squad coordination ----------------------

	/**
	 * Radius 안에 있는 같은 팀 적에게 NewTarget을 전파.
	 * Combat 진입 시 호출하여 동료를 함께 활성화.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Combat|Squad")
	void NotifyAlliesInRadius(float Radius, AActor* SharedTarget);

	// ---------------------- Delegates ----------------------

	UPROPERTY(BlueprintAssignable, Category = "PD|Combat")
	FPDOnTargetChanged OnTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Combat")
	FPDOnAttackRequested OnAttackRequested;

protected:
	/** 공격 쿨다운(초). RequestAttack 성공 후 이 시간 동안 재공격 불가. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Combat", meta = (ClampMin = "0.0"))
	float AttackCooldown = 1.0f;

	/** RequestAttack을 시도할 때 타겟까지의 최대 거리. 초과 시 실패. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 1500.f;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;

	/** 마지막 RequestAttack 성공 시각(WorldTimeSeconds). */
	float LastAttackTime = -1.f;
};
