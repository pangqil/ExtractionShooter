#include "Components/PDLocomotionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UPDLocomotionComponent::UPDLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		CachedMovement = OwnerCharacter->GetCharacterMovement();
	}

	UpdateMaxWalkSpeed();
}

void UPDLocomotionComponent::SetGait(EPDGait NewGait)
{
	if (CurrentGait == NewGait) return;

	CurrentGait = NewGait;
	UpdateMaxWalkSpeed();
	OnGaitChanged.Broadcast(NewGait);
}

void UPDLocomotionComponent::SetAimState(EPDAimState NewState)
{
	if (CurrentAimState == NewState) return;

	//조준 시 walk전환
	if (NewState == EPDAimState::Aim && CurrentGait == EPDGait::Run)
	{
		SetGait(EPDGait::Walk);
	}

	CurrentAimState = NewState;
	UpdateMaxWalkSpeed();
	OnAimStateChanged.Broadcast(NewState);
}

void UPDLocomotionComponent::StartRun()
{
	//조준 중이면 Run안됨
	if (CurrentAimState == EPDAimState::Aim) return;

	SetGait(EPDGait::Run);
}

void UPDLocomotionComponent::StopRun()
{
	if (CurrentGait == EPDGait::Run)
	{
		SetGait(EPDGait::Walk);
	}
}

void UPDLocomotionComponent::UpdateMaxWalkSpeed()
{
	if (!CachedMovement) return;
	CachedMovement->MaxWalkSpeed = ComputeTargetSpeed();
}

float UPDLocomotionComponent::ComputeTargetSpeed() const
{
	float BaseSpeed = WalkSpeed;
	switch (CurrentGait)
	{
	case EPDGait::Sneak: BaseSpeed = SneakSpeed; break;
	case EPDGait::Walk:  BaseSpeed = WalkSpeed;  break;
	case EPDGait::Run:   BaseSpeed = RunSpeed;   break;
	}

	//조준 시 속도 감소
	if (CurrentAimState == EPDAimState::Aim)
	{
		BaseSpeed *= AimSpeedMultiplier;
	}

	return BaseSpeed;
}