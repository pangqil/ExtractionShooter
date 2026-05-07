// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "Interfaces/PDDamageable.h"
#include "PDBaseNoiseEmitter.generated.h"

class UAIPerceptionStimuliSourceComponent;
class UStateTreeComponent;

/**
 * 청각 미끼 + 타겟 더미 겸용 액터.
 *  - 청각: BeginPlay 에 PerceptionStimuliSource 로 Hearing 등록 → EmitNoise() 호출 시 ReportNoiseEvent.
 *  - 시각: 동일 컴포넌트에 Sight 도 등록 → Soldier 가 쳐다보면 HandleTargetSpotted 자극 발생.
 *  - 데미지: IPDDamageable 구현 — 가상 체력 차감, 0 이하면 Sight 가시성 제거.
 *
 * Senior 관점: "소리 내는 액터" 와 "맞는 더미" 는 본래 SRP 상 분리 대상이지만,
 *              테스트 단계에서 양쪽을 같은 액터로 검증하기 위해 합쳐둠. 추후 별도
 *              APDTargetDummy 클래스로 분리 검토.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDBaseNoiseEmitter : public AActor,
	public IGenericTeamAgentInterface,
	public IPDDamageable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APDBaseNoiseEmitter();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DummyTarget", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BaseMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DummyTarget", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* DummyMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DummyTarget", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionStimuliSourceComponent* PerceptionStimuliSourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DummyTarget", meta = (AllowPrivateAccess = "true"))
	UStateTreeComponent* StateTreeComponent;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NoiseGeneration")
	float MaxNoiseRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NoiseGeneration")
	float NoiseRangeRandomDeviation = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NoiseGeneration")
	FName NoiseTag = "EmitterNoise";

	/**
	 * Soldier(TeamID=2) 가 이 액터를 "적"으로 분류해서 듣게 하려면 다른 TeamID(예: 0)로 둔다.
	 * 디폴트 attitude solver: 같은 ID = Friendly, 다른 ID = Hostile, NoTeam = Neutral.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	uint8 TeamID = 0;

	/** 더미 가상 체력. ApplyDamage 가 차감, 0 이하면 IsAlive() 가 false → Combat 측 invalid 처리. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Damage", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PD|Damage")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Damage", meta = (ClampMin = "0.0"))
	float SightOffsetZ = 50.f;

	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamID); }

	UFUNCTION(BlueprintCallable)
	void EmitNoise();

	// IPDDamageable
	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;
	virtual float GetCurrentHealth_Implementation() const override { return CurrentHealth; }
	virtual float GetMaxHealth_Implementation() const override { return MaxHealth; }
	virtual bool IsAlive_Implementation() const override { return CurrentHealth > 0.f; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
};
