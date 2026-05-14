#include "Ability/PDCoverAbility.h"
#include "Component/PDCoverComponent.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameplayTag/PDGameplayTags.h"

UPDCoverAbility::UPDCoverAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 구르는 중엔 엄폐 불가
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
}

void UPDCoverAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UPDCoverComponent* CoverComp = GetCoverComponent();
	if (!CoverComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (CoverComp->IsInCover())
		CoverComp->ExitCover();
	else
		CoverComp->TryEnterCover();

	// 컴포넌트가 상태를 관리하므로 어빌리티는 즉시 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPDCoverComponent* UPDCoverAbility::GetCoverComponent() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return nullptr;
	return Avatar->FindComponentByClass<UPDCoverComponent>();
}
