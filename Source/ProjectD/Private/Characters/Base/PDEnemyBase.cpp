#include "Characters/Base/PDEnemyBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AttributeSet/PDAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Items/World/PDItemBase.h"
#include "Items/World/PDLootItem.h"
#include "Items/World/PDLootBoxActor.h"
#include "Items/Containers/PDLootComponent.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Component/PDWeaponComponent.h"
#include "Data/PDQuestComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Widgets/Enemy/PDEnemyOverheadWidget.h"

APDEnemyBase::APDEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// ??AI ??Minimal �?충분. (???�이?�는 ?�스?�만 보유)
	if (ASC)
	{
		ASC->SetIsReplicated(true);
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}

	// Hostile 기본�? ?�식(?? PDSoldier)?�서 ?�정??가??
	TeamID = 2;

	// 머리 ??HUD ??Screen 공간 ?�젯. ?�드 좌표�??�면???�영??2D ?�버?�이�?그리므�?World ?��?비용????��(머터리얼·?�더?�깃·뎁??처리 ?�음).
	// 주의: ?�면공간?�라 ??�� 카메?��? ?�하�??�클루전???�용?��? ?�으�? 로컬 PlayerController 기�??�로�??�더?�다.
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawAtDesiredSize(true);
	OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
	OverheadWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	OverheadWidgetComponent->SetGenerateOverlapEvents(false);
	OverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDEnemyBase, CurrentState);
}

void APDEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// ?�수�?개인 조�? ?�향 ???�스?�스마다 ??�?굴려 게임 ?�내 고정.
	// Pitch ??Offset(무기 forward?�몸??보정) 중심??±Max ?�포. Yaw ??0 중심??±Max ?�포.
	AimBias.Pitch = AimBiasPitchOffset
		+ (AimBiasMaxPitchDegrees > 0.f ? FMath::FRandRange(-AimBiasMaxPitchDegrees, AimBiasMaxPitchDegrees) : 0.f);
	if (AimBiasMaxYawDegrees > 0.f)
		AimBias.Yaw = FMath::FRandRange(-AimBiasMaxYawDegrees, AimBiasMaxYawDegrees);

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

	// ?�버?�드 HUD 초기?????�젯 ?�래??미�?????캐시??nullptr �??�아 모든 Show* ?�출??no-op.
	if (OverheadWidgetComponent)
	{
		if (OverheadWidgetClass)
		{
			OverheadWidgetComponent->SetWidgetClass(OverheadWidgetClass);
		}
		OverheadWidgetComponent->InitWidget();
		OverheadWidget = Cast<UPDEnemyOverheadWidget>(OverheadWidgetComponent->GetUserWidgetObject());
	}

	// Torso HP 변�?구독 + 초기�?push. ShowHealth() ?�까지 ?�젯?� collapsed ??�?push ??백그?�운??갱신.
	if (ASC && AttributeSet)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetTorsoHPAttribute())
			.AddUObject(this, &APDEnemyBase::OnTorsoHPChanged);

		if (OverheadWidget)
		{
			OverheadWidget->SetHealth(AttributeSet->GetTorsoHP(), AttributeSet->GetMaxTorsoHP());
		}
	}
}

void APDEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickFootstep(DeltaSeconds);
}

void APDEnemyBase::TickFootstep(float DeltaSeconds)
{
	if (!FootstepSound || CurrentState == EPDEnemyState::Dead) return;

	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || !MoveComp->IsMovingOnGround()) return;

	const float Speed = GetVelocity().Size2D();
	if (Speed < FootstepMinSpeed)
	{
		// ?��? 중엔 ?�적�?리셋 ???�음 출발 직후 즉시 발자�???�� 방�?.
		FootstepDistanceAccumulator = 0.f;
		return;
	}

	FootstepDistanceAccumulator += Speed * DeltaSeconds;
	if (FootstepDistanceAccumulator < FootstepStrideDistance) return;

	FootstepDistanceAccumulator = 0.f;

	const FVector Loc = GetActorLocation();
	UGameplayStatics::PlaySoundAtLocation(
		this, FootstepSound, Loc,
		1.f, 1.f, 0.f, SoundAttenuation);

	// AI �?��???�이�???0 ?�면 ?�킵.
	if (FootstepNoiseRange > 0.f)
	{
		MakeNoise(1.f, this, Loc, FootstepNoiseRange);
	}
}

void APDEnemyBase::OnTorsoHPChanged(const FOnAttributeChangeData& Data)
{
	if (!OverheadWidget || !AttributeSet) return;

	const float Max = AttributeSet->GetMaxTorsoHP();
	OverheadWidget->SetHealth(Data.NewValue, Max);

	// 첫 피해 시 HP 바 영구 노출. 어트리뷰트 복제로 서버/클라 양쪽에서 발화 → 클라 위젯도 켜짐.
	if (!bHealthBarShown && Data.NewValue < Max)
	{
		bHealthBarShown = true;
		OverheadWidget->ShowHealth();
	}
}


uint8 APDEnemyBase::GetTeamID_Implementation() const
{
	return TeamID;
}

EPDStaminaStatus APDEnemyBase::GetStaminaStatus_Implementation() const
{
	// �?Biped ?�폴?? Biped ?�식 ?�래?��? AttributeSet 기반?�로 ?�제 ?�태 반환.
	return EPDStaminaStatus::None;
}

void APDEnemyBase::SetEnemyState(EPDEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;

	// 게임플레이 훅(서버 권위): 루트 드랍·콜리전·타이머 정리 등. 원격 클라에선 실행되지 않음.
	switch (NewState)
	{
	case EPDEnemyState::Idle:   OnEnterState_Idle();   break;
	case EPDEnemyState::Alert:  OnEnterState_Alert();  break;
	case EPDEnemyState::Chase:  OnEnterState_Chase();  break;
	case EPDEnemyState::Combat: OnEnterState_Combat(); break;
	case EPDEnemyState::Dead:   OnEnterState_Dead();   break;
	default: break;
	}

	// 연출은 서버(리슨 호스트 포함)에서 즉시, 원격 클라는 CurrentState 복제 후 OnRep_EnemyState 에서.
	ApplyEnemyStateCosmetics(NewState);
}

void APDEnemyBase::OnRep_EnemyState(EPDEnemyState /*OldState*/)
{
	ApplyEnemyStateCosmetics(CurrentState);
}

void APDEnemyBase::ApplyEnemyStateCosmetics(EPDEnemyState NewState)
{
	OnEnemyStateChanged(NewState);

	// Dead ???�젯 ?�체 ?��?(?�체 ??HP ?�존 방�?). �??�엔 ?�태 말풍??갱신.
	if (OverheadWidget)
	{
		if (NewState == EPDEnemyState::Dead)
		{
			OverheadWidget->HideAll();
		}
		else
		{
			OverheadWidget->ShowSpeech(NewState, SpeechBubbleDuration);
		}
	}

	// 사망 몽타주·사운드: 자동 복제되지 않으므로 상태 복제 시점에 각 머신이 직접 재생.
	if (NewState == EPDEnemyState::Dead)
	{
		if (DeathMontage)
		{
			PlayAnimMontage(DeathMontage);
		}
		if (DeathSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, DeathSound, GetActorLocation(),
				1.f, 1.f, 0.f, SoundAttenuation);
		}
	}
}

void APDEnemyBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	Super::ApplyDamage_Implementation(DamageInfo);

	// ?�격?? ?�아?�을 ?�만 ?�생. ?�망 ?��?지??OnEnterState_Dead ??DeathSound �??�원??
	if (HurtSound && IsAlive_Implementation())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, HurtSound, GetActorLocation(),
			1.f, 1.f, 0.f, SoundAttenuation);
	}

<<<<<<< HEAD
	// �??�격 ???�스�??�구 ?�출. ?�후 ?�출?� ?�래그로 ?�락.
	if (!bHealthBarShown && OverheadWidget)
	{
		bHealthBarShown = true;
		OverheadWidget->ShowHealth();
	}
=======
	// 헬스바 노출은 OnTorsoHPChanged 가 담당(서버/클라 공통). 여기선 별도 처리 없음.
>>>>>>> 38770d40d9a455202ce835c7763a06378389963a

	// Combat/Dead??BT가 ?��? ?�동 ?�??중이거나 종료???�태 ??추적 ?�호 무시.
	if (CurrentState == EPDEnemyState::Combat || CurrentState == EPDEnemyState::Dead) return;
	if (!IsAlive_Implementation()) return;

	AActor* InstigatorActor = DamageInfo.Instigator.Get();
	if (!InstigatorActor) return;

	// Controller가 Instigator�??�어??경우 Pawn?�로 보정 (?�치 ?�뢰???�보).
	if (AController* InstController = Cast<AController>(InstigatorActor))
	{
		InstigatorActor = InstController->GetPawn();
		if (!InstigatorActor) return;
	}

	// 같�? ?�??가???��?지??추적 ?�리�?무시 (?�발/AOE ??.
	if (const IGenericTeamAgentInterface* InstTeam = Cast<IGenericTeamAgentInterface>(InstigatorActor))
	{
		if (InstTeam->GetGenericTeamId() == GetGenericTeamId()) return;
	}

	const FVector HintLocation = InstigatorActor->GetActorLocation();

	if (UPDCombatComponent* Combat = FindComponentByClass<UPDCombatComponent>())
	{
		Combat->SetLastNoiseLocation(InstigatorActor, HintLocation);
	}

	// BB 직접 갱신 ??AIController??OnNoiseHintChanged�?구독?��? ?�으므�?HandleNoiseHeard?� ?�일 ?�턴?�로 채운??
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
	// 충돌/?�동 ?�리. ?�체가 발사체�? 막�? ?�도�? (StimuliSource ?�제??베이??HandleDeath ?�서.)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// ?�망 ???�랍 ?�름 ???�서 중요:
	//  1) ?�체 컨테?�너(LootBox) 먼�? ?�폰 ???�속 ?�계가 LootBox ??LootComponent ??직접 ?�재.
	//  2) LootTable ?�이??추�? (LootBox ?�선, 컨테?�너 ?�으�??�드 ?�터�??�백).
	//  3) ?�착 무기 ?�률 굴려 LootBox ??추�?.
	SpawnCorpseContainer();
	DropLootOnDeath();
	TryDropEquippedWeaponToCorpse();

<<<<<<< HEAD
	// ?�망 모션 ?�생. ?�니메이?�이 ?�는 경우?�도 문제 ?�도�?null 체크.
	if(DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	// ?�망?? SetLifeSpan ?�로 ?�터가 �??�멸?��?�?PlaySoundAtLocation ?�로 detach ?�생.
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, DeathSound, GetActorLocation(),
			1.f, 1.f, 0.f, SoundAttenuation);
	}
=======
	// 사망 몽타주·사운드는 ApplyEnemyStateCosmetics 로 이동 — 서버/클라 모두에서 재생되도록.
>>>>>>> 38770d40d9a455202ce835c7763a06378389963a

	// 0 ?�하�??�구 보존(?�체 컨테?�너�??�체되???�나리오 ?? ??�??�에??LifeSpan ?�약.
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
		if (UPDQuestComponent* QuestComponent = QuestOwner->FindComponentByClass<UPDQuestComponent>())
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

	// AI ?�사결정 즉시 ?��? ??컨트롤러??보존(?�망 몽�?�??�속 처리 ?�전), ?�터 ?�멸 ??OnUnPossess 가 ?�괄 ?�리.
	if (APDEnemyAIControllerBase* AICon = Cast<APDEnemyAIControllerBase>(GetController()))
	{
		AICon->NotifyPawnDied();
	}
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

	// LootBox 가 ?�체 컨테?�너�??�폰???�으�?LootBox ??LootComponent �?직접 ?�재.
	// 컨테?�너가 LootBox 가 ?�니거나 ?�으�??�드 ?�터�??�백.
	UPDLootComponent* CorpseLoot = nullptr;
	if (APDLootBoxActor* Corpse = Cast<APDLootBoxActor>(GetCorpseContainer()))
	{
		CorpseLoot = Corpse->GetLootComponent();
	}

	TArray<AActor*> SpawnedWorldActors;
	const FVector Origin = GetActorLocation();

	for (const FPDLootEntry& Entry : LootTable)
	{
		if (FMath::FRand() > Entry.DropChance) continue;

		const int32 MinQ = FMath::Max(1, Entry.MinQuantity);
		const int32 MaxQ = FMath::Max(MinQ, Entry.MaxQuantity);
		const int32 Quantity = FMath::RandRange(MinQ, MaxQ);

		// LootBox ?�선 ??ItemID �??�으�?컴포?�트가 DT 조회?�서 처리.
		if (CorpseLoot && !Entry.ItemID.IsNone())
		{
			const bool bAdded = CorpseLoot->AddItemByID(Entry.ItemID, Quantity);
			if (!bAdded)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[%s] DropLootOnDeath: LootComponent->AddItemByID failed for ItemID=%s. "
					     "LootComponent ??ItemDataTable ???�정???�고 ?�당 row 가 존재?�는지 ?�인."),
					*GetName(), *Entry.ItemID.ToString());
			}
			continue;
		}

		// ?�백: ?�드 ?�터 ?�폰 (LootBox 컨테?�너 ?�거??ItemID 미설????.
		if (!Entry.ItemClass) continue;

		const FVector Offset = (LootSpawnRadius > 0.f)
			? FMath::VRand() * FMath::FRandRange(0.f, LootSpawnRadius)
			: FVector::ZeroVector;

		const FTransform SpawnXform(FRotator::ZeroRotator, Origin + FVector(Offset.X, Offset.Y, 0.f));

		APDLootItem* Item = World->SpawnActorDeferred<APDLootItem>(
			Entry.ItemClass,
			SpawnXform,
			/*Owner=*/this,
			/*Instigator=*/nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (Item)
		{
			if (!Entry.ItemID.IsNone())
			{
				Item->SetItemID(Entry.ItemID);
			}
			Item->Quantity = Quantity;
			Item->FinishSpawning(SpawnXform);
			SpawnedWorldActors.Add(Item);
		}
	}

	if (SpawnedWorldActors.Num() > 0)
	{
		OnLootDropped(SpawnedWorldActors);
	}
}

void APDEnemyBase::TryDropEquippedWeaponToCorpse()
{
	const FName WeaponID = GetEquippedWeaponItemID();
	if (WeaponID.IsNone()) return;

	// ?�률 굴림 ???�패 ??무기???�체 ?�터 ?�멸 ??같이 ?�라�?(?�식 OnEnterState_Dead ?�서 Destroy).
	if (FMath::FRand() > WeaponDropChance)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[%s] Weapon drop rolled out ??ItemID=%s, Chance=%.2f"),
			*GetName(), *WeaponID.ToString(), WeaponDropChance);
		return;
	}

	APDLootBoxActor* Corpse = Cast<APDLootBoxActor>(GetCorpseContainer());
	if (!Corpse)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] TryDropEquippedWeaponToCorpse: CorpseContainer is not APDLootBoxActor. Check CorpseContainerClass."),
			*GetName());
		return;
	}

	UPDLootComponent* Loot = Corpse->GetLootComponent();
	if (!Loot) return;

	const bool bAdded = Loot->AddItemByID(WeaponID, 1);
	if (!bAdded)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] Weapon drop AddItemByID failed ??ItemID=%s. "
			     "LootComponent ??ItemDataTable 미설???�는 DT row 부??"),
			*GetName(), *WeaponID.ToString());
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
