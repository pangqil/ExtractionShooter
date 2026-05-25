#include "Ability/GA_RollAbility.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTag/PDGameplayTags.h"
#include "TimerManager.h"

UGA_RollAbility::UGA_RollAbility()
{
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
}

void UGA_RollAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APDCharacterBase* Character=GetPDCharacter();
	UAbilitySystemComponent* ASC=GetAbilitySystemComponentFromActorInfo();
	const UPDAttributeSet* AttributeSet = GetAttributeSet();

	if (!Character || Character->IsDowned() || Character->IsGettingUp() || Character->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[PD RollCostTrace] Activate before commit. Ability=%s Character=%s Authority=%d ASC=%s CostGE=%s Stamina=%.2f MaxStamina=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(Character),
		Character ? Character->HasAuthority() : false,
		*GetNameSafe(ASC),
		*GetNameSafe(CostGameplayEffectClass ? CostGameplayEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr),
		AttributeSet ? AttributeSet->GetStamina() : -1.f,
		AttributeSet ? AttributeSet->GetMaxStamina() : -1.f);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PD RollCostTrace] Commit failed. Ability=%s Character=%s Authority=%d CostGE=%s Stamina=%.2f MaxStamina=%.2f"),
			*GetNameSafe(this),
			*GetNameSafe(Character),
			Character ? Character->HasAuthority() : false,
			*GetNameSafe(CostGameplayEffectClass ? CostGameplayEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr),
			AttributeSet ? AttributeSet->GetStamina() : -1.f,
			AttributeSet ? AttributeSet->GetMaxStamina() : -1.f);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[PD RollCostTrace] Commit succeeded. Ability=%s Character=%s Authority=%d CostGE=%s Stamina=%.2f MaxStamina=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(Character),
		Character ? Character->HasAuthority() : false,
		*GetNameSafe(CostGameplayEffectClass ? CostGameplayEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr),
		AttributeSet ? AttributeSet->GetStamina() : -1.f,
		AttributeSet ? AttributeSet->GetMaxStamina() : -1.f);

	const FVector RollDir = GetRollDirection();
	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->Velocity.Z = 0.f;
		MovementComponent->SetPlaneConstraintEnabled(true);
		MovementComponent->SetPlaneConstraintNormal(FVector::UpVector);
	}
	StartRollGroundClamp(Character);

	if (!RollDir.IsNearlyZero())
	{
		const FRotator RollRotation(0.f, RollDir.Rotation().Yaw, 0.f);
		Character->SetActorRotation(RollRotation);
	}

	if (ASC && RollSound)
	{
		FGameplayCueParameters Params;
		Params.Location = Character->GetActorLocation();
		Params.TargetAttachComponent = Character->GetMesh();
		Params.SourceObject = RollSound;
		ASC->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Character_Roll, Params);
	}

	BP_OnActivate(RollDir);
}

void UGA_RollAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	StopRollGroundClamp();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UGA_RollAbility::FinishRoll()
{
	ClampRollGrounding();
	if (APDCharacterBase* Character = GetPDCharacter())
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->Velocity.Z = 0.f;
			if (MovementComponent->IsFalling())
			{
				MovementComponent->SetMovementMode(MOVE_Walking);
			}
		}
	}
	StopRollGroundClamp();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


FVector UGA_RollAbility::GetRollDirection() const
{
	APDCharacterBase* Character=GetPDCharacter();
	if (!Character) return FVector::ForwardVector;

	if (bUseInputDirection)
	{
		const FVector Input=Character->GetCharacterMovement()->GetLastInputVector();
		if (!Input.IsNearlyZero())
			return Input.GetSafeNormal2D();
	}

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	return Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
}

void UGA_RollAbility::StartRollGroundClamp(APDCharacterBase* Character)
{
	StopRollGroundClamp();
	if (!Character) return;

	RollGroundZ = Character->GetActorLocation().Z;
	bHasRollGroundZ = true;

	if (UWorld* World = Character->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RollGroundClampTimerHandle,
			this,
			&UGA_RollAbility::ClampRollGrounding,
			FMath::Max(0.001f, RollGroundClampInterval),
			true);
	}
}

void UGA_RollAbility::StopRollGroundClamp()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RollGroundClampTimerHandle);
	}
	bHasRollGroundZ = false;
}

void UGA_RollAbility::ClampRollGrounding()
{
	APDCharacterBase* Character = GetPDCharacter();
	if (!Character) return;

	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->Velocity.Z = 0.f;
		if (MovementComponent->IsFalling())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
		MovementComponent->SetPlaneConstraintEnabled(true);
		MovementComponent->SetPlaneConstraintNormal(FVector::UpVector);
	}

	if (!bHasRollGroundZ) return;

	const FVector CurrentLocation = Character->GetActorLocation();
	if (CurrentLocation.Z > RollGroundZ + MaxRollVerticalDrift)
	{
		Character->SetActorLocation(
			FVector(CurrentLocation.X, CurrentLocation.Y, RollGroundZ),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}
}
