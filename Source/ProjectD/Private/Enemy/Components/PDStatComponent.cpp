#include "Enemy/Components/PDStatComponent.h"

UPDStatComponent::UPDStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDStatComponent::BeginPlay()
{
	Super::BeginPlay();

	FScopeLock Lock(&BatteryCS);
	MaxBattery = FMath::Max(0.f, MaxBattery);
	CurrentBattery = FMath::Clamp(InitialBattery, 0.f, MaxBattery);
	CachedBatteryStatus = bUseBattery ? ComputeBatteryStatus(GetBatteryPercent()) : EPDBatteryStatus::None;
}

void UPDStatComponent::SetUseBattery(bool bInUseBattery)
{
	if (bUseBattery == bInUseBattery) return;

	bUseBattery = bInUseBattery;

	const EPDBatteryStatus NewStatus = bUseBattery ? ComputeBatteryStatus(GetBatteryPercent()) : EPDBatteryStatus::None;
	const EPDBatteryStatus OldStatus = CachedBatteryStatus;
	if (OldStatus != NewStatus)
	{
		CachedBatteryStatus = NewStatus;
		OnBatteryStatusChanged.Broadcast(OldStatus, NewStatus);
	}
}

float UPDStatComponent::GetCurrentBattery() const
{
	if (!bUseBattery) return 0.f;
	FScopeLock Lock(&BatteryCS);
	return CurrentBattery;
}

float UPDStatComponent::GetMaxBattery() const
{
	FScopeLock Lock(&BatteryCS);
	return MaxBattery;
}

float UPDStatComponent::GetBatteryPercent() const
{
	FScopeLock Lock(&BatteryCS);
	if (!bUseBattery || MaxBattery <= 0.f) return 0.f;
	return CurrentBattery / MaxBattery;
}

EPDBatteryStatus UPDStatComponent::GetBatteryStatus() const
{
	if (!bUseBattery) return EPDBatteryStatus::None;
	FScopeLock Lock(&BatteryCS);
	return CachedBatteryStatus;
}

void UPDStatComponent::SetBattery(float NewValue)
{
	if (!bUseBattery) return;
	ApplyBatteryChange_Internal(NewValue);
}

void UPDStatComponent::SetMaxBattery(float NewMax)
{
	if (NewMax < 0.f) NewMax = 0.f;

	float ClampedCurrent = 0.f;
	{
		FScopeLock Lock(&BatteryCS);
		MaxBattery = NewMax;
		ClampedCurrent = FMath::Clamp(CurrentBattery, 0.f, MaxBattery);
	}
	ApplyBatteryChange_Internal(ClampedCurrent);
}

bool UPDStatComponent::ConsumeBattery(float Amount)
{
	if (!bUseBattery || Amount <= 0.f) return false;

	float NewValue = 0.f;
	bool bFullySatisfied = false;
	{
		FScopeLock Lock(&BatteryCS);
		bFullySatisfied = (CurrentBattery >= Amount);
		NewValue = FMath::Max(0.f, CurrentBattery - Amount);
	}
	ApplyBatteryChange_Internal(NewValue);
	return bFullySatisfied;
}

void UPDStatComponent::RecoverBattery(float Amount)
{
	if (!bUseBattery || Amount <= 0.f) return;

	float NewValue = 0.f;
	{
		FScopeLock Lock(&BatteryCS);
		NewValue = FMath::Clamp(CurrentBattery + Amount, 0.f, MaxBattery);
	}
	ApplyBatteryChange_Internal(NewValue);
}

EPDBatteryStatus UPDStatComponent::ComputeBatteryStatus(float Percent) const
{
	if (Percent >= FullThreshold)    return EPDBatteryStatus::Full;
	if (Percent >= OptimalThreshold) return EPDBatteryStatus::Optimal;
	if (Percent >= LowThreshold)     return EPDBatteryStatus::Low;
	return EPDBatteryStatus::Exhausted;
}

void UPDStatComponent::ApplyBatteryChange_Internal(float NewValue)
{
	float Old = 0.f;
	float NewClamped = 0.f;
	float MaxRef = 0.f;
	EPDBatteryStatus OldStatus = EPDBatteryStatus::None;
	EPDBatteryStatus NewStatus = EPDBatteryStatus::None;
	bool bDepletedNow = false;

	{
		FScopeLock Lock(&BatteryCS);
		MaxRef = MaxBattery;
		Old = CurrentBattery;
		NewClamped = FMath::Clamp(NewValue, 0.f, MaxRef);

		if (FMath::IsNearlyEqual(Old, NewClamped))
		{
			return;
		}

		CurrentBattery = NewClamped;

		OldStatus = CachedBatteryStatus;
		const float Percent = (MaxRef > 0.f) ? (NewClamped / MaxRef) : 0.f;
		NewStatus = bUseBattery ? ComputeBatteryStatus(Percent) : EPDBatteryStatus::None;
		CachedBatteryStatus = NewStatus;

		bDepletedNow = (NewClamped <= 0.f && Old > 0.f);
	}

	// 락 밖에서 broadcast — 구독자가 본 컴포넌트를 다시 호출해도 데드락 없도록.
	OnBatteryChanged.Broadcast(NewClamped, MaxRef);

	if (OldStatus != NewStatus)
	{
		OnBatteryStatusChanged.Broadcast(OldStatus, NewStatus);
	}

	if (bDepletedNow)
	{
		OnBatteryDepleted.Broadcast();
	}
}
