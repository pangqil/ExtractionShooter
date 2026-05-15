#include "Enemy/Characters/PDScavenger.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "GameFramework/Character.h"
#include "Interfaces/PDDamageable.h"

APDScavenger::APDScavenger()
{
	TeamID = 2; // Hostile.
}

void APDScavenger::BeginPlay()
{
	Super::BeginPlay();

	// Soldier 와 동일 흐름: CombatComponent 가 보낸 공격 의도 → 자체 몽타주 재생.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &APDScavenger::HandleAttackRequested);
	}
}

void APDScavenger::HandleAttackRequested(AActor* Target)
{
	// 트레이스 시점에 다시 조회하지 않도록 공격 의도 시점의 타겟을 약참조로 보관.
	CachedAttackTarget = Target;

	if (!bAutoPlayMontageOnAttackRequested) return;
}

void APDScavenger::PerformMeleeTrace()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	UWorld* World = GetWorld();
	if (!MeshComp || !World) return;

	const FVector Start = MeshComp->DoesSocketExist(MeleeSocketName)
		? MeshComp->GetSocketLocation(MeleeSocketName)
		: GetActorLocation();

	// 기본은 정면. 캐시된 타겟의 head 소켓이 있으면 그쪽으로 보정.
	FVector Direction = GetActorForwardVector();
	if (const ACharacter* TargetChar = Cast<ACharacter>(CachedAttackTarget.Get()))
	{
		const USkeletalMeshComponent* TargetMesh = TargetChar->GetMesh();
		if (TargetMesh && TargetMesh->DoesSocketExist(TargetHeadSocketName))
		{
			const FVector ToHead = TargetMesh->GetSocketLocation(TargetHeadSocketName) - Start;
			if (!ToHead.IsNearlyZero())
			{
				Direction = ToHead.GetSafeNormal();
			}
		}
	}
	const FVector End = Start + Direction * MeleeTraceDistance;

	TArray<FHitResult> Hits;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(MeleeTraceRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_ScavengerMelee), false, this);
	Params.AddIgnoredActor(this);

	const bool bHit = World->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		Params);

	if (!bHit) return;

	const uint8 OwnTeam = TeamID;

	// 한 번의 스윕에서 동일 액터 중복 인가 방지.
	TSet<AActor*> AlreadyHit;
	AlreadyHit.Add(this);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target) continue;
		if (AlreadyHit.Contains(Target)) continue;
		AlreadyHit.Add(Target);

		// 같은 팀(아군) 제외.
		if (Target->Implements<UPDCombatInterface>()
			&& IPDCombatInterface::Execute_GetTeamID(Target) == OwnTeam)
		{
			continue;
		}

		if (!Target->Implements<UPDDamageable>()) continue;
		if (!IPDDamageable::Execute_IsAlive(Target)) continue;

		FPDDamageInfo Info;
		Info.BaseDamage = MeleeDamage;
		Info.Instigator = this;
		Info.DamageTypeClass = MeleeDamageTypeClass;
		Info.HitResult = Hit;
		IPDDamageable::Execute_ApplyDamage(Target, Info);
	}
}
