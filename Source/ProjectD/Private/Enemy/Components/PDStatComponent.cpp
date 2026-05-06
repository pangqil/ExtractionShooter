#include "Enemy/Components/PDStatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet/PDAttributeSet.h"
#include "GameFramework/Actor.h"

UPDStatComponent::UPDStatComponent()
{
	// Junior: Tick 끄기. 본 컴포넌트는 이벤트 기반(콜백 + delegate broadcast).
	// Mid: TickComponent 비활성화로 매 프레임 비용 절약.
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Battery 초기값 세팅 (락 안에서 일괄 처리)
	{
		FScopeLock Lock(&BatteryCS);
		MaxBattery = FMath::Max(0.f, MaxBattery);
		CurrentBattery = FMath::Clamp(InitialBattery, 0.f, MaxBattery);
		CachedBatteryStatus = bUseBattery ? ComputeBatteryStatus(GetBatteryPercent()) : EPDBatteryStatus::None;
	}

	// Senior: GAS 초기화는 부모(APDCharacterBase::BeginPlay)에서 이뤄짐.
	// 컴포넌트 BeginPlay는 부모 액터 BeginPlay 이후 호출됨이 보장되지 않으므로,
	// 안전을 위해 약간 지연시켜 ASC가 초기화된 다음에 바인딩.
	if (AActor* Owner = GetOwner())
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle BindHandle;
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UPDStatComponent::BindToAbilitySystem)
			);
		}
	}
}

void UPDStatComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	UnbindFromAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void UPDStatComponent::SetUseBattery(bool bInUseBattery)
{
	if (bUseBattery == bInUseBattery)
	{
		return;
	}

	bUseBattery = bInUseBattery;

	const EPDBatteryStatus NewStatus = bUseBattery ? ComputeBatteryStatus(GetBatteryPercent()) : EPDBatteryStatus::None;
	const EPDBatteryStatus OldStatus = CachedBatteryStatus;
	if (OldStatus != NewStatus)
	{
		CachedBatteryStatus = NewStatus;
		OnBatteryStatusChanged.Broadcast(OldStatus, NewStatus);
	}
}

float UPDStatComponent::GetCurrentHealth() const
{
	if (!CachedASC.IsValid())
	{
		return 0.f;
	}

	if (const UPDAttributeSet* Set = Cast<UPDAttributeSet>(CachedASC->GetAttributeSet(UPDAttributeSet::StaticClass())))
	{
		return Set->GetTorsoHP();
	}
	return 0.f;
}

float UPDStatComponent::GetMaxHealth() const
{
	if (!CachedASC.IsValid())
	{
		return 0.f;
	}

	if (const UPDAttributeSet* Set = Cast<UPDAttributeSet>(CachedASC->GetAttributeSet(UPDAttributeSet::StaticClass())))
	{
		return Set->GetMaxTorsoHP();
	}
	return 0.f;
}

bool UPDStatComponent::IsAlive() const
{
	return GetCurrentHealth() > 0.f;
}

float UPDStatComponent::GetCurrentBattery() const
{
	if (!bUseBattery)
	{
		return 0.f;
	}
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
	if (!bUseBattery || MaxBattery <= 0.f)
	{
		return 0.f;
	}
	return CurrentBattery / MaxBattery;
}

EPDBatteryStatus UPDStatComponent::GetBatteryStatus() const
{
	if (!bUseBattery)
	{
		return EPDBatteryStatus::None;
	}
	// CachedBatteryStatus는 ApplyBatteryChange_Internal에서만 갱신.
	FScopeLock Lock(&BatteryCS);
	return CachedBatteryStatus;
}

void UPDStatComponent::SetBattery(float NewValue)
{
	if (!bUseBattery)
	{
		return;
	}
	ApplyBatteryChange_Internal(NewValue);
}

void UPDStatComponent::SetMaxBattery(float NewMax)
{
	if (NewMax < 0.f)
	{
		NewMax = 0.f;
	}

	float ClampedCurrent = 0.f;
	{
		FScopeLock Lock(&BatteryCS);
		MaxBattery = NewMax;
		ClampedCurrent = FMath::Clamp(CurrentBattery, 0.f, MaxBattery);
	}
	// 값이 새 max에 의해 변경됐다면 일반 경로로 broadcast.
	ApplyBatteryChange_Internal(ClampedCurrent);
}

bool UPDStatComponent::ConsumeBattery(float Amount)
{
	if (!bUseBattery || Amount <= 0.f)
	{
		return false;
	}

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
	if (!bUseBattery || Amount <= 0.f)
	{
		return;
	}

	float NewValue = 0.f;
	{
		FScopeLock Lock(&BatteryCS);
		NewValue = FMath::Clamp(CurrentBattery + Amount, 0.f, MaxBattery);
	}
	ApplyBatteryChange_Internal(NewValue);
}

void UPDStatComponent::OnTorsoHPChangedNative(const FOnAttributeChangeData& Data)
{
	const float NewHP = Data.NewValue;
	const float MaxHP = GetMaxHealth();

	OnHealthChanged.Broadcast(NewHP, MaxHP);

	// HP=0 도달 1회만 broadcast.
	if (NewHP <= 0.f && !bHealthDepletedFired)
	{
		bHealthDepletedFired = true;
		OnHealthDepleted.Broadcast();
	}
	else if (NewHP > 0.f && bHealthDepletedFired)
	{
		// 부활/리셋 시나리오 대비.
		bHealthDepletedFired = false;
	}
}

void UPDStatComponent::BindToAbilitySystem()
{
	if (CachedASC.IsValid())
	{
		return; // 이미 바인딩됨.
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : Owner->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		// Senior: ASC가 아직 준비되지 않은 경우(원격 클라이언트 등) — 1프레임 더 대기.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UPDStatComponent::BindToAbilitySystem)
			);
		}
		return;
	}

	CachedASC = ASC;
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetTorsoHPAttribute())
		.AddUObject(this, &UPDStatComponent::OnTorsoHPChangedNative);

	// 초기값을 한번 broadcast하여 UI/StateTree가 동기화되도록 함.
	OnHealthChanged.Broadcast(GetCurrentHealth(), GetMaxHealth());
}

void UPDStatComponent::UnbindFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetTorsoHPAttribute()).RemoveAll(this);
	}
	CachedASC.Reset();
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
			return; // 변화 없음 — broadcast 생략.
		}

		CurrentBattery = NewClamped;

		OldStatus = CachedBatteryStatus;
		const float Percent = (MaxRef > 0.f) ? (NewClamped / MaxRef) : 0.f;
		NewStatus = bUseBattery ? ComputeBatteryStatus(Percent) : EPDBatteryStatus::None;
		CachedBatteryStatus = NewStatus;

		bDepletedNow = (NewClamped <= 0.f && Old > 0.f);
	}

	// Mid: broadcast는 락 밖에서 — 구독자가 다시 본 컴포넌트를 호출해도 데드락 없도록.
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
