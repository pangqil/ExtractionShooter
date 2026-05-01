#include "Characters/PDCharacterBase.h"

APDCharacterBase::APDCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APDCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void APDCharacterBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	if (!IsAlive_Implementation()) return;

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageInfo.BaseDamage, 0.f, MaxHealth);

	if (!IsAlive_Implementation())
	{
		HandleDeath(DamageInfo.Instigator.Get());
	}
}

void APDCharacterBase::HandleDeath(AActor* Killer)
{
	OnDeathDelegate.Broadcast(Killer);
	OnDeath(Killer);
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
