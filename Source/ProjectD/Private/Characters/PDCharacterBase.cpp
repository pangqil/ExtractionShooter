#include "Characters/PDCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Type/Types.h"

APDCharacterBase::APDCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AttributeSet = CreateDefaultSubobject<UPDAttributeSet>(TEXT("AttributeSet"));
}

void APDCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystem();
}

void APDCharacterBase::InitAbilitySystem()
{
	if (!ASC) return;

	ASC->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		InitializeAttributes();
		GiveStartupAbilities();
	}
}

void APDCharacterBase::InitializeAttributes()
{
	if (!ASC || !DefaultAttributes) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DefaultAttributes, 1.f, Context);
	if (!Spec.IsValid()) return;

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void APDCharacterBase::GiveStartupAbilities()
{
	if (!ASC) return;

	for (TSubclassOf<UGameplayAbility> Ability : StartupAbilities)
	{
		if (Ability) ASC->GiveAbility(FGameplayAbilitySpec(Ability, 1));
	}
}

void APDCharacterBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
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
	return GetCurrentHealth_Implementation() > 0.f;
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
