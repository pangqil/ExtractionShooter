#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "PDScavenger.generated.h"

class UAnimMontage;
class UDamageType;

/**
 * 근접 공격형 적 (Scavenger).
 *  - APDSoldier 와 같은 BipedEnemy 계층의 형제 클래스. 행동(BT/Perception) 은 Soldier 와 동일하게 BP 측에서 구성.
 *  - 무기 액터 미보유. CombatComponent.OnAttackRequested → AttackMontage 재생,
 *    BP AnimNotify 가 PerformMeleeTrace() 호출하여 소켓 기준 SphereTrace + IPDDamageable 데미지 인가.
 *  - AttackRange 는 BP 디테일의 CombatComponent->AttackRange 에서 150 권장 (근접 사거리).
 *
 * 확장 포인트:
 *  - 다단 히트박스: BP 에서 AnimNotify 시점마다 PerformMeleeTrace() 를 여러 번 호출.
 *  - 동작 분리: bAutoPlayMontageOnAttackRequested=false 후 BP 측 OnAttackRequested 에서 직접 제어.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDScavenger : public APDBipedEnemy
{
	GENERATED_BODY()

public:
	APDScavenger();

	/** 캐릭터 메시의 소켓 기준 SphereTrace → 적대 IPDDamageable 에 데미지 인가. BP AnimNotify 에서 호출. */
	UFUNCTION(BlueprintCallable, Category = "PD|Scavenger|Melee")
	void PerformMeleeTrace();

protected:
	virtual void BeginPlay() override;

	/** 공격 모션. BP 디자이너가 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** 트레이스 시작 소켓 (캐릭터 메시). 손/머리/입 등 공격 부위. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee")
	FName MeleeSocketName = TEXT("hand_r");

	/** 타겟 메시에서 조준 기준이 되는 본/소켓 이름. UE5 manny 기본은 "head". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee")
	FName TargetHeadSocketName = TEXT("head");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee", meta = (ClampMin = "0.0"))
	float MeleeTraceRadius = 35.f;

	/** 소켓에서 조준 방향으로 뻗는 트레이스 길이. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee", meta = (ClampMin = "0.0"))
	float MeleeTraceDistance = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee", meta = (ClampMin = "0.0"))
	float MeleeDamage = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee")
	TSubclassOf<UDamageType> MeleeDamageTypeClass;

	/** OnAttackRequested 시 AttackMontage 자동 재생 여부. false 면 BP 가 OnAttackRequested 처리. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Scavenger|Melee")
	bool bAutoPlayMontageOnAttackRequested = true;

private:
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);

	/** OnAttackRequested 시점의 타겟 — PerformMeleeTrace 가 머리 방향 보정에 사용. */
	TWeakObjectPtr<AActor> CachedAttackTarget;
};
