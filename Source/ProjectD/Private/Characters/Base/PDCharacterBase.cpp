#include "Characters/Base/PDCharacterBase.h"
#include "Characters/PDPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PDGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Net/UnrealNetwork.h"
#include "Type/Types.h"

namespace
{
	int32 SendReviveGameplayEvent(AActor* Reviver, AActor* Target, const FGameplayTag& EventTag)
	{
		UAbilitySystemComponent* ReviverASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Reviver);
		if (!ReviverASC || !EventTag.IsValid())
		{
			return 0;
		}

		FGameplayEventData EventData;
		EventData.EventTag = EventTag;
		EventData.Instigator = Reviver;
		EventData.Target = Target;
		return ReviverASC->HandleGameplayEvent(EventTag, &EventData);
	}
}

APDCharacterBase::APDCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	ASC=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet=CreateDefaultSubobject<UPDAttributeSet>(TEXT("AttributeSet"));


	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	if (StimuliSource)
	{
		StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
		StimuliSource->bAutoRegister = true;
	}

	WeaponComponent = CreateDefaultSubobject<UPDWeaponComponent>(TEXT("WeaponComponent"));
}

void APDCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDCharacterBase, LifeState);
	DOREPLIFETIME(APDCharacterBase, DownedExpireServerTime);
	DOREPLIFETIME(APDCharacterBase, ActiveReviver);
	DOREPLIFETIME(APDCharacterBase, ReviveEndServerTime);
}

void APDCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystem();
	SyncLifeStateTags();

}

void APDCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetOwner(NewController);
	InitAbilitySystem();
}

void APDCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitAbilitySystem();
}

void APDCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	UE_LOG(LogTemp, Warning,
		TEXT("[PD MoveSpeedRoot] OnMoveSpeedChanged. Character=%s Authority=%d Old=%.2f New=%.2f CurrentMaxWalk=%.2f LifeState=%d"),
		*GetNameSafe(this),
		HasAuthority() ? 1 : 0,
		Data.OldValue,
		Data.NewValue,
		MovementComponent ? MovementComponent->MaxWalkSpeed : -1.f,
		static_cast<int32>(LifeState));

	if (LifeState == EPDLifeState::Downed)
	{
		GetCharacterMovement()->MaxWalkSpeed = DownedMoveSpeed;
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed=Data.NewValue;
}

void APDCharacterBase::InitAbilitySystem()
{
	if (!ASC) return;
	ASC->InitAbilityActorInfo(this, this);

	if (!bAbilitySystemDelegatesBound)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &APDCharacterBase::OnMoveSpeedChanged);
		bAbilitySystemDelegatesBound = true;
	}

	InitializeAttributes();
	GiveStartupAbilities();
	GiveActiveAbilities();
}

void APDCharacterBase::InitializeAttributes()
{
	if (!HasAuthority()) return;
	if (bAttributesInitialized) return;
	if (!ASC || !DefaultAttributes)
	{
		return;
	}
	FGameplayEffectContextHandle ContextHandle=ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle=ASC->MakeOutgoingSpec(DefaultAttributes, 1.f, ContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (AttributeSet)
	{
		AttributeSet->SetHeadHP (AttributeSet->GetMaxHeadHP());
		AttributeSet->SetTorsoHP(AttributeSet->GetMaxTorsoHP());
		AttributeSet->SetArmLHP (AttributeSet->GetMaxArmLHP());
		AttributeSet->SetArmRHP (AttributeSet->GetMaxArmRHP());
		AttributeSet->SetLegLHP (AttributeSet->GetMaxLegLHP());
		AttributeSet->SetLegRHP (AttributeSet->GetMaxLegRHP());
		AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
		AttributeSet->SetHunger(AttributeSet->GetMaxHunger());
		AttributeSet->SetThirst(AttributeSet->GetMaxThirst());
		AttributeSet->SetGasMask(AttributeSet->GetMaxGasMask());
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
		}
		AttributeSet->bIsInitialized=true;
	}
	bAttributesInitialized = true;
}

void APDCharacterBase::GiveStartupAbilities()
{
	if (!HasAuthority()) return;
	if (bStartupAbilitiesGiven) return;
	if (!ASC) return;
	for (const auto& StartupAbility : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(StartupAbility);
		ASC->GiveAbility(AbilitySpec);
	}
	bStartupAbilitiesGiven = true;
}

void APDCharacterBase::GiveActiveAbilities()
{
	if (!HasAuthority()) return;
	if (bActiveAbilitiesGiven) return;
	if (!ASC) return;
	for (const auto& ActiveAbility : ActiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(ActiveAbility);
		ASC->GiveAbility(AbilitySpec);
	}
	bActiveAbilitiesGiven = true;
}

void APDCharacterBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	if (!HasAuthority()) return;
	if (!ASC) return;

	FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
	Context.AddHitResult(DamageInfo.HitResult);
	Context.AddInstigator(DamageInfo.Instigator.Get(), DamageInfo.Instigator.Get());

	FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(PDGameplayTags::Data_Damage, DamageInfo.BaseDamage);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

float APDCharacterBase::GetCurrentHealth_Implementation() const
{
	if (AttributeSet) return AttributeSet->GetTorsoHP();
	return 0.f;
}

float APDCharacterBase::GetMaxHealth_Implementation() const
{
	if (AttributeSet) return AttributeSet->GetMaxTorsoHP();
	return 0.f;
}

bool APDCharacterBase::IsAlive_Implementation() const
{
	return LifeState == EPDLifeState::Alive && GetCurrentHealth_Implementation() > 0.f;
}

float APDCharacterBase::GetDownedRemainingTime() const
{
	if (LifeState != EPDLifeState::Downed || DownedExpireServerTime <= 0.f)
	{
		return 0.f;
	}

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float CurrentServerTime = GameState
		? GameState->GetServerWorldTimeSeconds()
		: (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	return FMath::Max(0.f, DownedExpireServerTime - CurrentServerTime);
}

float APDCharacterBase::GetReviveRemainingTime() const
{
	if (LifeState != EPDLifeState::Downed || ReviveEndServerTime <= 0.f)
	{
		return 0.f;
	}

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float CurrentServerTime = GameState
		? GameState->GetServerWorldTimeSeconds()
		: (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	return FMath::Max(0.f, ReviveEndServerTime - CurrentServerTime);
}

float APDCharacterBase::GetReviveProgress() const
{
	const float Duration = FMath::Max(0.f, ReviveTime);
	if (Duration <= KINDA_SMALL_NUMBER || !IsBeingRevived())
	{
		return 0.f;
	}

	return FMath::Clamp(1.f - (GetReviveRemainingTime() / Duration), 0.f, 1.f);
}

bool APDCharacterBase::CanBeKilledWhileDownedByDamage() const
{
	if (LifeState != EPDLifeState::Downed)
	{
		return false;
	}

	const float GracePeriod = FMath::Max(0.f, DownedDamageGracePeriod);
	if (GracePeriod <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float CurrentServerTime = GameState
		? GameState->GetServerWorldTimeSeconds()
		: (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	return DownedEnteredServerTime > 0.f && CurrentServerTime - DownedEnteredServerTime >= GracePeriod;
}

void APDCharacterBase::Interact_Implementation(AActor* Interactor)
{
	if (!HasAuthority()) return;
	if (LifeState != EPDLifeState::Downed)
	{
		return;
	}
	if (SendReviveGameplayEvent(Interactor, this, PDGameplayTags::Event_Revive_Start) <= 0)
	{
		BeginReviveInteraction(Interactor);
	}
}

bool APDCharacterBase::BeginReviveInteraction(AActor* Reviver)
{
	if (!HasAuthority()) return false;
	if (!IsValidReviver(Reviver)) return false;

	if (ActiveReviver.Get() && ActiveReviver.Get() != Reviver)
	{
		return false;
	}

	const float Duration = FMath::Max(0.1f, ReviveTime);
	ActiveReviver = Reviver;
	ReviveStartLocation = Reviver->GetActorLocation();

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float CurrentServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	ReviveEndServerTime = CurrentServerTime + Duration;

	ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		ReviveTimerHandle,
		this,
		&APDCharacterBase::TickReviveInteraction,
		FMath::Max(0.01f, ReviveValidationInterval),
		true);

	return true;
}

void APDCharacterBase::CancelReviveInteraction(AActor* Reviver)
{
	if (!HasAuthority()) return;
	if (!ActiveReviver.Get()) return;
	if (Reviver && ActiveReviver.Get() != Reviver) return;

	SendReviveGameplayEvent(ActiveReviver.Get(), this, PDGameplayTags::Event_Revive_Cancel);
	ClearReviveInteraction(true);
}

void APDCharacterBase::SetLifeState(EPDLifeState NewLifeState, AActor* ContextActor)
{
	if (LifeState == NewLifeState) return;

	const EPDLifeState OldLifeState = LifeState;
	LifeState = NewLifeState;
	bIsDead = LifeState == EPDLifeState::Dead;
	ForceNetUpdate();
	OnLifeStateChanged(OldLifeState, ContextActor);
}

void APDCharacterBase::HandleDowned(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;
	if (LifeState != EPDLifeState::Alive)
	{
		return;
	}

	ClearReviveInteraction(false);
	PendingGetUpContext = nullptr;
	if (ASC)
	{
		FGameplayTagContainer BleedingTags;
		BleedingTags.AddTag(PDGameplayTags::State_Debuff_Bleeding);
		ASC->RemoveActiveEffectsWithGrantedTags(BleedingTags);
		if (ASC->GetTagCount(PDGameplayTags::State_Debuff_Bleeding) > 0)
		{
			ASC->RemoveLooseGameplayTag(PDGameplayTags::State_Debuff_Bleeding);
		}
	}
	SetLifeState(EPDLifeState::Downed, InstigatorActor);
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	DownedEnteredServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	StartDownedDeathTimer();
}

void APDCharacterBase::HandleDeath(AActor* Killer)
{
	if (!HasAuthority()) return;
	if (LifeState == EPDLifeState::Dead) return;

	ClearReviveInteraction(false);
	ClearDownedDeathTimer();
	PendingGetUpContext = nullptr;
	DownedEnteredServerTime = 0.f;

	SetLifeState(EPDLifeState::Dead, Killer);


	if (StimuliSource)
	{
		StimuliSource->UnregisterFromPerceptionSystem();
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
		{
			GM->OnPlayerDied(PC, Killer);
		}
	}
}

void APDCharacterBase::Revive(AActor* Reviver)
{
	if (!HasAuthority()) return;
	if (LifeState != EPDLifeState::Downed) return;
	if (!Reviver || Reviver == this) return;
	if (!IsValidReviver(Reviver)) return;

	ClearReviveInteraction(false);
	ClearDownedDeathTimer();
	PendingGetUpContext = nullptr;
	DownedEnteredServerTime = 0.f;
	ApplyReviveHealth(Reviver);
	if (AttributeSet && (AttributeSet->GetHeadHP() <= 0.f || AttributeSet->GetTorsoHP() <= 0.f))
	{
		StartDownedDeathTimer();
		return;
	}
	PendingGetUpContext = Reviver;
	SetLifeState(EPDLifeState::GettingUp, Reviver);
}

void APDCharacterBase::FinishGettingUp()
{
	if (!HasAuthority()) return;
	if (LifeState != EPDLifeState::GettingUp) return;

	AActor* ContextActor = PendingGetUpContext.Get();
	PendingGetUpContext = nullptr;
	SetLifeState(EPDLifeState::Alive, ContextActor);
}

void APDCharacterBase::ResetLifeStateToAlive(AActor* ContextActor)
{
	if (!HasAuthority()) return;

	ClearReviveInteraction(false);
	ClearDownedDeathTimer();
	PendingGetUpContext = nullptr;
	DownedEnteredServerTime = 0.f;

	if (LifeState != EPDLifeState::Alive)
	{
		SetLifeState(EPDLifeState::Alive, ContextActor);
		return;
	}

	bIsDead = false;
	OnLifeStateChanged(EPDLifeState::Alive, ContextActor);
	ForceNetUpdate();
}

void APDCharacterBase::OnRep_LifeState(EPDLifeState OldLifeState)
{
	bIsDead = LifeState == EPDLifeState::Dead;
	OnLifeStateChanged(OldLifeState, nullptr);
}

void APDCharacterBase::OnLifeStateChanged(EPDLifeState OldLifeState, AActor* ContextActor)
{
	SyncLifeStateTags();
	ApplyLifeStateMovement();

	if (LifeState == EPDLifeState::Downed)
	{
		OnDowned(ContextActor);
		return;
	}

	if (LifeState == EPDLifeState::Alive)
	{
		RestoreAliveCollisionAndInput();
		if (OldLifeState == EPDLifeState::Downed || OldLifeState == EPDLifeState::GettingUp)
		{
			OnRevived(ContextActor);
		}
		return;
	}

	if (LifeState == EPDLifeState::GettingUp)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->DisableMovement();
		MovementComponent->StopMovementImmediately();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->IsLocalController())
		{
			DisableInput(PlayerController);
		}
	}

	OnDeathDelegate.Broadcast(ContextActor);
	OnDeath(ContextActor);
}

void APDCharacterBase::SyncLifeStateTags()
{
	if (!ASC) return;

	ASC->SetLooseGameplayTagCount(PDGameplayTags::State_Alive, 0);
	ASC->SetLooseGameplayTagCount(PDGameplayTags::State_Downed, 0);
	ASC->SetLooseGameplayTagCount(PDGameplayTags::State_GettingUp, 0);
	ASC->SetLooseGameplayTagCount(PDGameplayTags::State_Dead, 0);

	switch (LifeState)
	{
	case EPDLifeState::Alive:
		ASC->AddLooseGameplayTag(PDGameplayTags::State_Alive);
		break;
	case EPDLifeState::Downed:
		ASC->AddLooseGameplayTag(PDGameplayTags::State_Downed);
		break;
	case EPDLifeState::GettingUp:
		ASC->AddLooseGameplayTag(PDGameplayTags::State_GettingUp);
		break;
	case EPDLifeState::Dead:
		ASC->AddLooseGameplayTag(PDGameplayTags::State_Dead);
		break;
	}
}

void APDCharacterBase::RestoreAliveCollisionAndInput()
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->IsLocalController())
		{
			EnableInput(PlayerController);
		}
	}
}

void APDCharacterBase::ApplyLifeStateMovement()
{
	if (!GetCharacterMovement()) return;

	if (LifeState == EPDLifeState::Downed || LifeState == EPDLifeState::GettingUp)
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		return;
	}

	if (LifeState == EPDLifeState::Alive && AttributeSet)
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}
}

void APDCharacterBase::ApplyReviveHealth(AActor* Reviver)
{
	if (!HasAuthority() || !ASC || !AttributeSet) return;

	if (!ReviveEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddInstigator(Reviver, Reviver);
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ReviveEffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void APDCharacterBase::StartDownedDeathTimer()
{
	if (!HasAuthority()) return;

	ClearDownedDeathTimer();

	if (DownedLifeSpan <= 0.f)
	{
		DownedExpireServerTime = 0.f;
		return;
	}

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float CurrentServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	DownedExpireServerTime = CurrentServerTime + DownedLifeSpan;

	GetWorldTimerManager().SetTimer(
		DownedDeathTimerHandle,
		this,
		&APDCharacterBase::HandleDownedExpired,
		DownedLifeSpan,
		false);
}

void APDCharacterBase::ClearDownedDeathTimer()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(DownedDeathTimerHandle);
	}
	DownedExpireServerTime = 0.f;
}

void APDCharacterBase::HandleDownedExpired()
{
	if (!HasAuthority()) return;
	if (LifeState != EPDLifeState::Downed) return;

	HandleDeath(nullptr);
}

void APDCharacterBase::CompleteReviveInteraction()
{
	if (!HasAuthority()) return;

	AActor* Reviver = ActiveReviver.Get();
	if (!IsValidReviver(Reviver))
	{
		ClearReviveInteraction(true);
		return;
	}

	GetWorldTimerManager().ClearTimer(ReviveTimerHandle);
	ActiveReviver = nullptr;
	ReviveEndServerTime = 0.f;
	ForceNetUpdate();
	Revive(Reviver);
}

void APDCharacterBase::TickReviveInteraction()
{
	if (!HasAuthority()) return;

	AActor* Reviver = ActiveReviver.Get();
	if (!IsValidReviver(Reviver))
	{
		ClearReviveInteraction(true);
		return;
	}

	const float MoveTolerance = FMath::Max(0.f, ReviveCancelMoveTolerance);
	if (MoveTolerance > 0.f &&
		FVector::DistSquared2D(Reviver->GetActorLocation(), ReviveStartLocation) > FMath::Square(MoveTolerance))
	{
		ClearReviveInteraction(true);
		return;
	}

	if (GetReviveRemainingTime() <= KINDA_SMALL_NUMBER)
	{
		CompleteReviveInteraction();
	}
}

void APDCharacterBase::ClearReviveInteraction(bool)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ReviveTimerHandle);
	}

	ActiveReviver = nullptr;
	ReviveEndServerTime = 0.f;
	ReviveStartLocation = FVector::ZeroVector;

	ForceNetUpdate();
}

bool APDCharacterBase::IsValidReviver(AActor* Reviver) const
{
	if (LifeState != EPDLifeState::Downed)
	{
		return false;
	}
	if (!Reviver || Reviver == this)
	{
		return false;
	}

	const APDCharacterBase* ReviverCharacter = Cast<APDCharacterBase>(Reviver);
	if (!ReviverCharacter || ReviverCharacter->LifeState != EPDLifeState::Alive)
	{
		return false;
	}

	const APDPlayerCharacter* ReviverPlayerCharacter = Cast<APDPlayerCharacter>(Reviver);
	if (!ReviverPlayerCharacter || !ReviverPlayerCharacter->IsInteractingWith(this))
	{
		return false;
	}

	const float MaxDistance = FMath::Max(0.f, ReviveInteractDistance);
	if (FVector::DistSquared(GetActorLocation(), Reviver->GetActorLocation()) > FMath::Square(MaxDistance))
	{
		return false;
	}

	return true;
}

void APDCharacterBase::AttachActorToWeaponSocket(AActor* ActorToAttach)
{
	if (!ActorToAttach) return;

	ActorToAttach->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponSocketName
	);
}
