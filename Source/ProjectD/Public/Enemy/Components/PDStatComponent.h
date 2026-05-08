#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDStatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnBatteryChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnBatteryStatusChanged, EPDBatteryStatus, OldStatus, EPDBatteryStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnBatteryDepleted);

/**
 * 적 전용 Battery(스태미너) 컴포넌트.
 * HP는 GAS PDAttributeSet이 단일 진실 원천이므로 본 컴포넌트는 손대지 않음.
 * Battery 만 별도로 두는 이유: 의미상 Player Stamina와 분리 + AttributeSet 비종속.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDStatComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetUseBattery(bool bInUseBattery);

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	FORCEINLINE bool IsUsingBattery() const { return bUseBattery; }

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	float GetCurrentBattery() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	float GetMaxBattery() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	float GetBatteryPercent() const;

	UFUNCTION(BlueprintPure, Category = "PD|Stat|Battery")
	EPDBatteryStatus GetBatteryStatus() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetBattery(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void SetMaxBattery(float NewMax);

	/** 양수만큼 소비. true=요청량 전부 소비 가능. */
	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	bool ConsumeBattery(float Amount);

	UFUNCTION(BlueprintCallable, Category = "PD|Stat|Battery")
	void RecoverBattery(float Amount);

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Battery")
	FPDOnBatteryChanged OnBatteryChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Battery")
	FPDOnBatteryStatusChanged OnBatteryStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stat|Battery")
	FPDOnBatteryDepleted OnBatteryDepleted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery")
	bool bUseBattery = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0"))
	float MaxBattery = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0"))
	float InitialBattery = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullThreshold = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OptimalThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Stat|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowThreshold = 0.2f;

private:
	EPDBatteryStatus ComputeBatteryStatus(float Percent) const;
	void ApplyBatteryChange_Internal(float NewValue);

	EPDBatteryStatus CachedBatteryStatus = EPDBatteryStatus::None;
	float CurrentBattery = 0.f;

	mutable FCriticalSection BatteryCS;
};
