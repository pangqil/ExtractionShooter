#include "Enemy/Characters/PDBipedEnemy.h"

#include "AttributeSet/PDAttributeSet.h"
#include "Enemy/Components/PDCombatComponent.h"

APDBipedEnemy::APDBipedEnemy()
{
	CombatComponent = CreateDefaultSubobject<UPDCombatComponent>(TEXT("CombatComponent"));
}

EPDStaminaStatus APDBipedEnemy::GetStaminaStatus_Implementation() const
{
	// AttributeSet 미초기화/Max=0 이면 Stamina 미사용으로 간주.
	if (!AttributeSet) return EPDStaminaStatus::None;

	const float MaxS = AttributeSet->GetMaxStamina();
	if (MaxS <= 0.f) return EPDStaminaStatus::None;

	const float Percent = AttributeSet->GetStamina() / MaxS;

	if (Percent >= StaminaFullThreshold)    return EPDStaminaStatus::Full;
	if (Percent >= StaminaOptimalThreshold) return EPDStaminaStatus::Optimal;
	if (Percent >= StaminaLowThreshold)     return EPDStaminaStatus::Low;
	return EPDStaminaStatus::Exhausted;
}
