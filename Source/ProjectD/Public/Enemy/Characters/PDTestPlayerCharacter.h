#pragma once

#include "CoreMinimal.h"
#include "Characters/PDPlayerCharacter.h"
#include "PDTestPlayerCharacter.generated.h"

class UGameplayEffect;

/**
 * 적 AI/전투 테스트 전용 플레이어.
 * - bGodMode 가 true 면 데미지 GE 적용 자체를 차단 (어트리뷰트 변동 없음 → HandleDeath 트리거 없음).
 * - InfiniteResourceEffectClass 는 Stamina/Hunger/Thirst 를 Max 로 유지하는 Periodic Infinite GE.
 *   본 게임 로직은 건드리지 않으므로 테스트 맵의 GameMode DefaultPawnClass 만 BP_TestPlayerCharacter 로 바꿔 사용.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDTestPlayerCharacter : public APDPlayerCharacter
{
	GENERATED_BODY()

public:
	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Test")
	void SetGodMode(bool bEnabled) { bGodMode = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "PD|Test")
	FORCEINLINE bool IsGodMode() const { return bGodMode; }

protected:
	virtual void InitAbilitySystem() override;

	/** true 면 모든 피격에서 데미지 GE 미적용. 런타임 토글 가능. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Test")
	bool bGodMode = true;

	/** Stamina/Hunger/Thirst 를 Max 로 강제하는 Infinite Periodic GE. BP 디폴트에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Test")
	TSubclassOf<UGameplayEffect> InfiniteResourceEffectClass;

	/** 무적 상태에서 피격이 들어온 순간 호출 — BP 에서 히트 피드백(사운드/셰이크) 작성. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Test")
	void OnGodModeHit(const FPDDamageInfo& DamageInfo);
};
