#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDStatComponent.generated.h"

class UAbilitySystemComponent;
class UPDAttributeSet;
struct FOnAttributeChangeData;

/** HP가 변경됐을 때 (현재값, 최대값) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnHealthChanged, float, NewValue, float, MaxValue);
/** HP가 0이 되어 사망 처리를 트리거해야 할 때 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnHealthDepleted);
/** Battery가 변경됐을 때 (현재값, 최대값) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnBatteryChanged, float, NewValue, float, MaxValue);
/** Battery 임계 구간이 바뀌었을 때 (Full↔Optimal↔Low↔Exhausted) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnBatteryStatusChanged, EPDBatteryStatus, OldStatus, EPDBatteryStatus, NewStatus);
/** Battery가 완전히 소진됐을 때 (Exhausted 진입) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnBatteryDepleted);

/**
 * Enemy의 HP / Battery(스태미너) 관리 컴포넌트.
 *
 * 책임 분담:
 *  - HP   : GAS UPDAttributeSet (TorsoHP)에 위임. 본 컴포넌트는 attribute 변화 delegate를
 *           구독하여 도메인 이벤트(Health depleted 등)로 변환만 담당.
 *  - Battery: 본 컴포넌트가 직접 소유. Player 측의 Stamina 어트리뷰트와 분리하여
 *             Enemy 시스템 독립성 확보.
 *
 * Senior 관점: HP를 GAS에 위임함으로써 GameplayEffect/Modifier/Replication을 그대로 활용,
 *              Battery는 Enemy 전용 자원이라 비교적 단순한 로컬 상태로 두어 GAS 오버헤드 회피.
 *              두 자원의 변경 알림을 모두 단일 컴포넌트에서 노출 → AI/StateTree 구독 지점 단일화.
 *
 * Mid 관점: Battery 접근은 FCriticalSection으로 보호 (UE 게임플레이는 단일 스레드가 일반적이지만,
 *           Animation/Particle/Async 작업에서 호출될 가능성을 대비). 변경 시에만 임계값 비교 후
 *           OnBatteryStatusChanged broadcast → 매 프레임 발사 방지.
 *
 * Junior 관점: 컴포넌트는 어디에든 Attach 가능하지만, Battery 사용은 bUseBattery=true 일 때만 의미.
 *              SetMaxBattery 후 SetBattery로 초기화하는 것이 안전.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStatComponent();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface

	// ---------------------- Battery 사용 가능 여부 ----------------------

	/** Battery 사용 여부. Biped 클래스 생성자에서 true로 설정. */
	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetUseBattery(bool bInUseBattery);

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	FORCEINLINE bool IsUsingBattery() const { return bUseBattery; }

	// ---------------------- Health (GAS 위임) ----------------------

	/** 현재 HP. ASC가 없으면 0. (TorsoHP 기준) */
	UFUNCTION(BlueprintPure, Category = "PD|Stat|Health")
	float GetCurrentHealth() const;

	/** 최대 HP. ASC가 없으면 0. */
	UFUNCTION(BlueprintPure, Category = "PD|Stat|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Health")
	bool IsAlive() const;

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

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Health")
	FPDOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Health")
	FPDOnHealthDepleted OnHealthDepleted;

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
	/** GAS 어트리뷰트(TorsoHP) 변화 콜백. */
	void OnTorsoHPChangedNative(const FOnAttributeChangeData& Data);

	/** Owner의 ASC를 찾아 어트리뷰트 변화 delegate를 구독. */
	void BindToAbilitySystem();
	void UnbindFromAbilitySystem();

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

	/** Bind 시 받은 핸들 (소유 ASC가 살아있는 동안 유효). */
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	/** HP=0 도달을 한 번만 broadcast하기 위한 가드. */
	bool bHealthDepletedFired = false;
};
