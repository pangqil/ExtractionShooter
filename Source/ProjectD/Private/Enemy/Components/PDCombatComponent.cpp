#include "Enemy/Components/PDCombatComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Enemy/Interfaces/PDCombatInterface.h"

UPDCombatComponent::UPDCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDCombatComponent::SetCurrentTarget(AActor* NewTarget)
{
	AActor* OldTarget = CurrentTarget.Get();
	if (OldTarget == NewTarget)
	{
		return;
	}
	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(NewTarget);
}

void UPDCombatComponent::ClearCurrentTarget()
{
	if (!CurrentTarget.IsValid() && CurrentTarget.IsExplicitlyNull())
	{
		return;
	}
	CurrentTarget.Reset();
	OnTargetChanged.Broadcast(nullptr);
}

bool UPDCombatComponent::HasValidTarget() const
{
	AActor* T = CurrentTarget.Get();
	if (!T)
	{
		return false;
	}

	// IPDCombatInterface가 있다면 IsAlive 검증 — 사망한 액터는 무효 타겟.
	if (T->Implements<UPDCombatInterface>())
	{
		return IPDCombatInterface::Execute_IsAlive(T);
	}
	return true;
}

bool UPDCombatComponent::CanAttack() const
{
	if (IsOnCooldown())            return false;
	if (!HasValidTarget())         return false;

	const AActor* Owner = GetOwner();
	const AActor* Target = CurrentTarget.Get();
	if (!Owner || !Target)         return false;

	const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (AttackRange * AttackRange);
}

bool UPDCombatComponent::RequestAttack()
{
	if (!CanAttack())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	LastAttackTime = World->GetTimeSeconds();

	// 의도만 broadcast. 실제 발사 로직은 디자이너/무기 컴포넌트.
	OnAttackRequested.Broadcast(CurrentTarget.Get());
	return true;
}

bool UPDCombatComponent::IsOnCooldown() const
{
	if (LastAttackTime < 0.f) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	return (World->GetTimeSeconds() - LastAttackTime) < AttackCooldown;
}

float UPDCombatComponent::GetCooldownRemaining() const
{
	if (LastAttackTime < 0.f) return 0.f;

	const UWorld* World = GetWorld();
	if (!World) return 0.f;

	const float Elapsed = World->GetTimeSeconds() - LastAttackTime;
	return FMath::Max(0.f, AttackCooldown - Elapsed);
}

void UPDCombatComponent::NotifyAlliesInRadius(float Radius, AActor* SharedTarget)
{
	if (Radius <= 0.f) return;

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) return;

	uint8 OwnerTeam = 0;
	if (Owner->Implements<UPDCombatInterface>())
	{
		OwnerTeam = IPDCombatInterface::Execute_GetTeamID(Owner);
	}

	// Senior: PhysicsAsset/CollisionProfile에 의존하지 않도록 광역 OverlapMultiByObjectType 대신
	//         단순 액터 이터레이션도 가능하지만, 대규모 레벨에서는 Overlap이 더 빠름.
	//         일단 ECC_Pawn 채널 기준으로 Pawn류 액터를 찾아 인터페이스 구현 + 같은 팀만 필터링.
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_NotifyAllies), false, Owner);

	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		Params
	);

	if (!bHit) return;

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* Other = Result.GetActor();
		if (!Other || Other == Owner) continue;
		if (!Other->Implements<UPDCombatInterface>()) continue;

		// 같은 팀만.
		if (IPDCombatInterface::Execute_GetTeamID(Other) != OwnerTeam) continue;

		// 살아있는 동료에게만.
		if (!IPDCombatInterface::Execute_IsAlive(Other)) continue;

		// 동료의 CombatComponent 찾기.
		UPDCombatComponent* AllyCombat = Other->FindComponentByClass<UPDCombatComponent>();
		if (!AllyCombat) continue;

		// 이미 같은 타겟이면 skip.
		if (AllyCombat->GetCurrentTarget() == SharedTarget) continue;

		AllyCombat->SetCurrentTarget(SharedTarget);
	}
}
