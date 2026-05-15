#include "Enemy/Characters/PDTestPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Interfaces/PDDamageable.h"

void APDTestPlayerCharacter::InitAbilitySystem()
{
	Super::InitAbilitySystem();

	// Stamina/Hunger/Thirst 무한 유지 — 부모의 HungerDecay/ThirstDecay 가 이미 적용된 뒤이므로
	// Periodic 회복 GE 가 매 틱 Max 로 덮어쓰는 형태로 동작.
	if (!ASC || !InfiniteResourceEffectClass) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(InfiniteResourceEffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void APDTestPlayerCharacter::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	// PDAttributeSet 의 사망 체크가 데미지 GE 적용 직후 같은 틱에 돌기 때문에,
	// Periodic 회복으로는 막을 수 없어 데미지 적용 자체를 차단한다.
	if (bGodMode)
	{
		OnGodModeHit(DamageInfo);
		return;
	}

	Super::ApplyDamage_Implementation(DamageInfo);
}
