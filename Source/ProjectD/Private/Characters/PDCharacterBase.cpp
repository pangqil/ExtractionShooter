#include "Characters/PDCharacterBase.h"
#include "AbilitySystemComponent.h"    
#include "GameplayEffectTypes.h"  
#include "GameplayTagContainer.h"
#include "Type/Types.h"

APDCharacterBase::APDCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APDCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void APDCharacterBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddHitResult(DamageInfo.HitResult);
	Context.AddInstigator(DamageInfo.Instigator.Get(), DamageInfo.Instigator.Get());

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), DamageInfo.BaseDamage);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

float APDCharacterBase::GetCurrentHealth_Implementation() const
{
	//TODO : AS Complete -> AttributeSet(HP Return)
	return 0.f;
}

bool APDCharacterBase::IsAlive_Implementation() const
{
	return GetCurrentHealth_Implementation()>0.f;
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
