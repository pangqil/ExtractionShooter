#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "PDPerceptionComponent.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

/** 새 자극이 감지되었거나 자극이 사라졌을 때. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnTargetSensed, AActor*, Actor, bool, bSensed);
/** 시야 안에서 적이 처음 발견됨. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnTargetSpotted, AActor*, Target);
/** 시야에서 적이 사라짐 (Lost). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnTargetLost, AActor*, Target, FVector, LastKnownLocation);
/** 의심스러운 소리/움직임을 들었음 (Alert 트리거). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnNoiseHeard, AActor*, NoiseInstigator, FVector, Location);

/**
 * UAIPerceptionComponent 래퍼.
 *  - 디자이너가 본 컴포넌트의 EditDefaultsOnly 값만 만지면 SightRadius/HearingRange 등이
 *    자동으로 SenseConfig에 적용됨.
 *  - 원시 OnTargetPerceptionUpdated를 도메인 이벤트(Spotted/Lost/NoiseHeard)로 변환하여
 *    StateTree/AIController가 구독.
 *
 * Senior 관점: UAIPerceptionComponent를 직접 상속함으로써 AIController가 자동 인식
 *              (FindComponentByClass<UAIPerceptionComponent>) — 별도 통합 코드 불필요.
 *              SenseConfig는 동적으로 생성하여 Default 클래스 설정과 PIE 설정이 충돌하지 않도록.
 *
 * Mid 관점: SenseConfig 갱신은 BeginPlay에서 한 번만. 매 프레임 갱신은 비용이 큼.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UPDPerceptionComponent();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnTargetSensed OnTargetSensed;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnTargetSpotted OnTargetSpotted;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnTargetLost OnTargetLost;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnNoiseHeard OnNoiseHeard;

	// ---------------------- Debug/외부 조회 ----------------------
	// SenseConfig 인스턴스는 private 라 외부에서 직접 못 봄.
	// AIController 의 디버그 시각화 등에서 BP defaults 값을 읽기 위한 getter.

	UFUNCTION(BlueprintPure, Category = "PD|Perception")
	float GetSightRadius() const { return SightRadius; }

	UFUNCTION(BlueprintPure, Category = "PD|Perception")
	float GetLoseSightRadius() const { return LoseSightRadius; }

	UFUNCTION(BlueprintPure, Category = "PD|Perception")
	float GetPeripheralVisionAngleDegrees() const { return PeripheralVisionAngleDegrees; }

	UFUNCTION(BlueprintPure, Category = "PD|Perception")
	float GetHearingRange() const { return HearingRange; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0"))
	float SightRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0"))
	float SightMaxAge = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Hearing", meta = (ClampMin = "0.0"))
	float HearingRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Hearing", meta = (ClampMin = "0.0"))
	float HearingMaxAge = 3.f;

	/** Sight/Hearing affiliation: 적팀(=Enemy)만 감지하도록 디자이너에게 노출. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Affiliation")
	bool bDetectEnemies = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Affiliation")
	bool bDetectNeutrals = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Affiliation")
	bool bDetectFriendlies = false;

private:
	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
};
