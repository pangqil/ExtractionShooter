#include "Characters/Base/PDEnemyBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Items/PDItemBase.h"
#include "Items/PDLootItem.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Component/PDWeaponComponent.h"
#include "Characters/PDPlayerCharacter.h"
#include "Data/PDQuestComponent.h"
#include "GameFramework/Controller.h"
#include "Weapons/Base/PDWeaponBase.h"

APDEnemyBase::APDEnemyBase()
{

	if (ASC)
	{
		ASC->SetIsReplicated(true);
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}


	TeamID = 2;
}

void APDEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (UPDCombatComponent* Combat = FindComponentByClass<UPDCombatComponent>())
		Combat->OnTargetChanged.AddDynamic(this, &APDEnemyBase::OnCombatTargetChanged);

	if (USkeletalMeshComponent* SkelMesh=GetMesh())
	{
		for (int32 i=0; i<SkelMesh->GetNumMaterials(); i++)
		{
			UMaterialInstanceDynamic* DynMat=SkelMesh->CreateAndSetMaterialInstanceDynamic(i);
			if (DynMat) DitherMaterials.Add(DynMat);
		}
	}
}


uint8 APDEnemyBase::GetTeamID_Implementation() const
{
	return TeamID;
}

EPDStaminaStatus APDEnemyBase::GetStaminaStatus_Implementation() const
{

	return EPDStaminaStatus::None;
}

void APDEnemyBase::SetEnemyState(EPDEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;

	switch (NewState)
	{
	case EPDEnemyState::Idle:   OnEnterState_Idle();   break;
	case EPDEnemyState::Alert:  OnEnterState_Alert();  break;
	case EPDEnemyState::Chase:  OnEnterState_Chase();  break;
	case EPDEnemyState::Combat: OnEnterState_Combat(); break;
	case EPDEnemyState::Dead:   OnEnterState_Dead();   break;
	default: break;
	}

	OnEnemyStateChanged(NewState);
}

void APDEnemyBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	Super::ApplyDamage_Implementation(DamageInfo);


	if (CurrentState == EPDEnemyState::Combat || CurrentState == EPDEnemyState::Dead) return;
	if (!IsAlive_Implementation()) return;

	AActor* InstigatorActor = DamageInfo.Instigator.Get();
	if (!InstigatorActor) return;


	if (AController* InstController = Cast<AController>(InstigatorActor))
	{
		InstigatorActor = InstController->GetPawn();
		if (!InstigatorActor) return;
	}


	if (const IGenericTeamAgentInterface* InstTeam = Cast<IGenericTeamAgentInterface>(InstigatorActor))
	{
		if (InstTeam->GetGenericTeamId() == GetGenericTeamId()) return;
	}

	const FVector HintLocation = InstigatorActor->GetActorLocation();

	if (UPDCombatComponent* Combat = FindComponentByClass<UPDCombatComponent>())
	{
		Combat->SetLastNoiseLocation(InstigatorActor, HintLocation);
	}


	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
		{
			BB->SetValueAsBool  (PDBTKeys::HasNoiseHint,      true);
			BB->SetValueAsVector(PDBTKeys::LastNoiseLocation, HintLocation);
		}
	}
}

void APDEnemyBase::OnEnterState_Dead()
{

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}


	DropLootOnDeath();
	SpawnCorpseContainer();


	if (CorpseDespawnDelay > 0.f)
	{
		SetLifeSpan(CorpseDespawnDelay);
	}
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);

	AActor* QuestOwner = Killer;
	if (AController* KillerController = Cast<AController>(Killer))
	{
		QuestOwner = KillerController->GetPawn();
	}

	if (QuestOwner)
	{
		UPDQuestComponent* QuestComponent = nullptr;
		if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(QuestOwner))
		{
			QuestComponent = PlayerCharacter->GetQuestComponent();
		}
		else
		{
			QuestComponent = QuestOwner->FindComponentByClass<UPDQuestComponent>();
		}

		if (QuestComponent)
		{
			const FName EnemyID = !QuestEnemyID.IsNone() ? QuestEnemyID : GetFName();
			QuestComponent->ReportEnemyKilled(EnemyID, 1);
			if (bIsQuestEnemy)
			{
				QuestComponent->ReportQuestEnemyKilled(EnemyID, 1);
			}
		}
	}

	Super::HandleDeath(Killer);
}

void APDEnemyBase::OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure)
{
	for (UMaterialInstanceDynamic* Mat : DitherMaterials)
	{
		if (Mat) Mat->SetScalarParameterValue(TEXT("Exposure"), Exposure);
	}
}

void APDEnemyBase::DropLootOnDeath()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> Spawned;
	const FVector Origin = GetActorLocation();

	for (const FPDLootEntry& Entry : LootTable)
	{
		if (!Entry.ItemClass) continue;
		if (FMath::FRand() > Entry.DropChance) continue;

		const int32 MinQ = FMath::Max(1, Entry.MinQuantity);
		const int32 MaxQ = FMath::Max(MinQ, Entry.MaxQuantity);
		const int32 Quantity = FMath::RandRange(MinQ, MaxQ);

		const FVector Offset = (LootSpawnRadius > 0.f)
			? FMath::VRand() * FMath::FRandRange(0.f, LootSpawnRadius)
			: FVector::ZeroVector;


		const FTransform SpawnXform(FRotator::ZeroRotator, Origin + FVector(Offset.X, Offset.Y, 0.f));

		APDLootItem* Item = World->SpawnActorDeferred<APDLootItem>(
			Entry.ItemClass,
			SpawnXform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (Item)
		{
			if (!Entry.ItemID.IsNone())
			{
				Item->SetItemID(Entry.ItemID);
			}
			Item->Quantity = Quantity;
			Item->FinishSpawning(SpawnXform);
			Spawned.Add(Item);
		}
	}

	if (Spawned.Num() > 0)
	{
		OnLootDropped(Spawned);
	}
}

AActor* APDEnemyBase::SpawnCorpseContainer()
{
	if (!CorpseContainerClass) return nullptr;

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	AActor* Spawned = World->SpawnActor<AActor>(
		CorpseContainerClass,
		GetActorLocation(),
		GetActorRotation(),
		Params);

	CorpseContainerInstance = Spawned;
	return Spawned;
}

void APDEnemyBase::SetAimTarget(AActor* Target)
{
	if (WeaponComponent)
		WeaponComponent->SetAimTarget(Target);
}

void APDEnemyBase::ClearAimTarget()
{
	if (WeaponComponent)
		WeaponComponent->ClearAimTarget();
}

void APDEnemyBase::OnCombatTargetChanged(AActor* NewTarget)
{
	if (NewTarget) SetAimTarget(NewTarget);
	else           ClearAimTarget();
}
