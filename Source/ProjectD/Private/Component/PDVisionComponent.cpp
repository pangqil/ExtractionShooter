#include "Component/PDVisionComponent.h"

#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDDetectable.h"
#include "Kismet/KismetMaterialLibrary.h"

UPDVisionComponent::UPDVisionComponent()
{
	// Transform(위치/방향)은 매 프레임 MPC 업데이트 필요.
	PrimaryComponentTick.bCanEverTick=true;
	PrimaryComponentTick.bStartWithTickEnabled=true;
}


void UPDVisionComponent::BeginPlay()
{
	Super::BeginPlay();
	LastLocation=GetOwner()->GetActorLocation();
	LastYaw=GetOwner()->GetActorRotation().Yaw;
	// VisionRange/AngleCos 초기값 한 번 밀어넣기
	UpdateFogOfWarMPC_Vision();
	ScheduleNextUpdate(UpdateInterval);
}

void UPDVisionComponent::EndPlay(EEndPlayReason::Type Reason)
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	for (AActor* Actor:VisibleActors)
	{
		if (IsValid(Actor)) UpdateExposure(Actor, 0.f);
	}
	VisibleActors.Empty();
	Super::EndPlay(Reason);
}

void UPDVisionComponent::BindToAttributeSet(UAbilitySystemComponent* ASC)
{
	if (!ASC) return;
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetVisionRangeAttribute())
		.AddUObject(this, &UPDVisionComponent::OnVisionRangeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetVisionAngleAttribute())
		.AddUObject(this, &UPDVisionComponent::OnVisionAngleChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetVisionUpdateIntervalAttribute())
		.AddUObject(this, &UPDVisionComponent::OnVisionUpdateIntervalChanged);
}

void UPDVisionComponent::SetVisionAngleOverride(float Angle)
{
	bHasAngleOverride=true;
	VisionAngleOverride=Angle;
}

void UPDVisionComponent::ClearVisionAngleOverride()
{
	bHasAngleOverride=false;
}

void UPDVisionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Transform만 매 프레임 MPC 업데이트 (VisionRange/Angle은 어트리뷰트 변경 시만)
	UpdateFogOfWarMPC_Transform();
}

void UPDVisionComponent::OnVisionRangeChanged(const FOnAttributeChangeData& Data)
{
	VisionRange=Data.NewValue;
	UpdateFogOfWarMPC_Vision();
}

void UPDVisionComponent::OnVisionAngleChanged(const FOnAttributeChangeData& Data)
{
	VisionAngle=Data.NewValue;
	UpdateFogOfWarMPC_Vision();
}

void UPDVisionComponent::OnVisionUpdateIntervalChanged(const FOnAttributeChangeData& Data)
{
	UpdateInterval=Data.NewValue;
}

void UPDVisionComponent::PerformVisionUpdate()
{
	AActor* Owner=GetOwner();
	if (!IsValid(Owner)) return;
	
	const FVector CurrentLocation=Owner->GetActorLocation();
	const float CurrentYaw=Owner->GetActorRotation().Yaw;
	
	const bool bMoved=FVector::Dist(CurrentLocation, LastLocation)>LocationThreshold;
	const bool bRotated=FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, LastYaw))>YawThreshold;
	const bool bMoving=bMoved||bRotated;
	
	if (bMoving)
	{
		LastLocation=CurrentLocation;
		LastYaw=CurrentYaw;
	}
	
	ScheduleNextUpdate(bMoving ? UpdateInterval : ThrottledInterval);
	
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	
	GetWorld()->OverlapMultiByChannel(Overlaps, CurrentLocation, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(VisionRange), Params);
	
	TSet<AActor*> NewVisible;
	for (const FOverlapResult& Result:Overlaps)
	{
		AActor* Target=Result.GetActor();
		if (!IsValid(Target)) continue;
		if (!Target->Implements<UPDDetectable>()) continue;
		if (!IsInCone(Target)) continue;
		if (!HasLineOfSight(Target)) continue;
		NewVisible.Add(Target);
	}
	for (AActor* Actor:VisibleActors)
	{
		if (!NewVisible.Contains(Actor)) UpdateExposure(Actor, 0.f);
	}
	for (AActor* Actor:NewVisible)
	{
		UpdateExposure(Actor, CalcExposure(Actor));
	}

	VisibleActors=MoveTemp(NewVisible);
	// Transform은 Tick에서 처리하므로 여기서는 Vision 파라미터(Range/Angle)만 동기화
	UpdateFogOfWarMPC_Vision();
}

float UPDVisionComponent::CalcExposure(AActor* Target) const
{
	const float Distance=FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	const float Linear=FMath::Clamp(1.f-(Distance/VisionRange), 0.f, 1.f);
	return Linear*Linear;
}


bool UPDVisionComponent::IsInCone(AActor* Target) const
{
	const FVector ToTargetRaw=Target->GetActorLocation()-GetOwner()->GetActorLocation();

	if (ToTargetRaw.SizeSquared()<FMath::Square(ProximityRadius)) return true;

	const FVector Forward=GetOwner()->GetActorForwardVector();
	const float Dot=FVector::DotProduct(Forward, ToTargetRaw.GetSafeNormal());
	return Dot>=FMath::Cos(FMath::DegreesToRadians(GetEffectiveAngle()*0.5f));
}

bool UPDVisionComponent::HasLineOfSight(AActor* Target) const
{
	FHitResult Hit;
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(Target);
	
	const bool bBlocked=GetWorld()->LineTraceSingleByChannel(Hit, GetOwner()->GetActorLocation(),
		Target->GetActorLocation(), ECC_GameTraceChannel1, Params);
	return !bBlocked;
}

void UPDVisionComponent::UpdateExposure(AActor* Target, float Exposure)
{
	IPDDetectable::Execute_OnVisionExposureChanged(Target, GetOwner(), Exposure);
}

void UPDVisionComponent::ScheduleNextUpdate(float Interval)
{
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this,
		&UPDVisionComponent::PerformVisionUpdate, Interval, false);
}

float UPDVisionComponent::GetEffectiveAngle() const
{
	return bHasAngleOverride?VisionAngleOverride : VisionAngle;
}

void UPDVisionComponent::UpdateStaminaScale(float Scale)
{
	StaminaScale=FMath::Clamp(Scale, 0.3f, 1.f);
	UpdateFogOfWarMPC_Vision();
}

void UPDVisionComponent::UpdateFogOfWarMPC_Transform()
{
	if (!FogOfWarMPC) return;
	AActor* Owner=GetOwner();
	if (!IsValid(Owner)) return;

	const FVector Loc=Owner->GetActorLocation();
	const FVector Forward=Owner->GetActorForwardVector();

	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogOfWarMPC,
		TEXT("PlayerWorldPosition"), FLinearColor(Loc.X, Loc.Y, Loc.Z, 0.f));
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogOfWarMPC,
		TEXT("PlayerForwardVector"), FLinearColor(Forward.X, Forward.Y, Forward.Z, 0.f));
}

void UPDVisionComponent::UpdateFogOfWarMPC_Vision()
{
	if (!FogOfWarMPC) return;

	const float AngleCos=FMath::Cos(FMath::DegreesToRadians(GetEffectiveAngle()*0.5f));

	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogOfWarMPC,
		TEXT("VisionRange"), VisionRange*StaminaScale);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogOfWarMPC,
		TEXT("VisionAngleCos"), AngleCos);
}



