#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDStatComponent.generated.h"

/** Battery가 변경됐을 때 (현재값, 최대값) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnBatteryChanged, float, NewValue, float, MaxValue);
/** Battery 임계 구간이 바뀌었을 때 (Full↔Optimal↔Low↔Exhausted) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnBatteryStatusChanged, EPDBatteryStatus, OldStatus, EPDBatteryStatus, NewStatus);
/** Battery가 완전히 소진됐을 때 (Exhausted 진입) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnBatteryDepleted);

/**
 * Enemy 전용 Battery(스태미너) 관리 컴포넌트.
 *
 * 책임:
 *  - Battery: 적 전용 자원. AttributeSet 에 없는 도메인이라 본 컴포넌트가 직접 소유.
 *  - 임계 구간(Full/Optimal/Low/Exhausted) 전이 감지 + delegate broadcast.
 *
 * Senior 관점: HP 는 GAS UPDAttributeSet (TorsoHP) 가 단일 진실 원천 — 본 컴포넌트는 손대지 않음.
 *              사망 처리는 UPDAttributeSet::PostGameplayEffectExecute 에서 HandleDeath 호출.
 *              Battery 만 따로 두는 이유: AttributeSet 의 Stamina 는 Player 자원이고
 *              Enemy 의 Battery 는 "동력 소진" 도메인 — 의미상 분리.
 *              (장기적으로 AttributeSet 통합도 가능. 현재는 SRP 우선.)
 *
 * Mid 관점: Battery 접근은 FCriticalSection 으로 보호 — Animation/Particle/Async 호출 대비.
 *           매 프레임 broadcast 방지를 위해 임계값 통과 시에만 OnBatteryStatusChanged 발사.
 *
 * Junior 관점: Battery 사용은 bUseBattery=true 일 때만 의미. SetMaxBattery 후 SetBattery 로 초기화.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStatComponent();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

	// ---------------------- Battery 사용 가능 여부 ----------------------

	/** Battery 사용 여부. Biped 클래스 생성자에서 true로 설정. */
	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetUseBattery(bool bInUseBattery);

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	FORCEINLINE bool IsUsingBattery() const { return bUseBattery; }

	// ---------------------- Battery (로컬 관리) ----------------------

	/** Battery 현재값. 비-Biped는 0. */
	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	float GetCurrentBattery() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	float GetMaxBattery() const;

	/** 0~1 비율. */
	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	float GetBatteryPercent() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	EPDBatteryStatus GetBatteryStatus() const;

	/** 절대값 설정 (clamped). */
	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetBattery(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetMaxBattery(float NewMax);

	/** 양수만큼 소비. 부족하면 0까지만. true=요청량 전부 소비 가능했음. */
	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	bool ConsumeBattery(float Amount);

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void RecoverBattery(float Amount);

	// ---------------------- Delegates ----------------------

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Battery")
	FPDOnBatteryChanged OnBatteryChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Battery")
	FPDOnBatteryStatusChanged OnBatteryStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Battery")
	FPDOnBatteryDepleted OnBatteryDepleted;

protected:
	/** Biped 등에서 true로 설정. 디폴트는 false. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery")
	bool bUseBattery = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0"))
	float MaxBattery = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0"))
	float InitialBattery = 100.f;

	/** 임계값(비율). 디자이너가 EnemyTypes 문서와 일치하도록 조정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullThreshold = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OptimalThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowThreshold = 0.2f;

private:
	/** 비율로부터 EPDBatteryStatus 계산. */
	EPDBatteryStatus ComputeBatteryStatus(float Percent) const;

	/**
	 * 현재 Battery 값을 NewValue로 갱신하고, 변경/임계 통과 시 delegate broadcast.
	 * 호출 측에서 락을 잡지 말 것 — 내부에서 잡음.
	 */
	void ApplyBatteryChange_Internal(float NewValue);

	/** Battery 상태 캐시. 임계값 통과 감지를 위함. */
	EPDBatteryStatus CachedBatteryStatus = EPDBatteryStatus::None;

	float CurrentBattery = 0.f;

	/** Battery 동시 접근 보호. UE 게임플레이는 단일 스레드가 보통이지만, 안전 차원에서 보유. */
	mutable FCriticalSection BatteryCS;
};
