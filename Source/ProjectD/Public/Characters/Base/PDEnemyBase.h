#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "PDEnemyBase.generated.h"

class UAIPerceptionStimuliSourceComponent;

/**
 * 모든 적의 추상 베이스 클래스.
 *
 * 책임:
 *  - EPDEnemyState 기반 FSM 상태 관리 (실제 전이 정책은 AIController/StateTree 측)
 *  - IPDCombatInterface 구현 (TeamID, IsAlive, GetBatteryStatus 기본값 None)
 *  - AI Perception 시스템에 자극원으로 등록 (UAIPerceptionStimuliSourceComponent)
 *  - 사망 처리: HandleDeath 에서 상태 Dead 전이 + 충돌 비활성/루트 hook
 *
 * Senior 관점: HP/Battery 관리는 자식(BipedEnemy)에서 컴포넌트로 추가하므로,
 *              본 클래스는 "모든 enemy"가 공통으로 가져야 할 최소한만 보유.
 *              Stim source는 모든 적이 가져야 하므로 여기서 생성.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase, public IPDCombatInterface
{
	GENERATED_BODY()

public:
	APDEnemyBase();

	//~ Begin IPDCombatInterface
	virtual uint8 GetTeamID_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;
	virtual EPDBatteryStatus GetBatteryStatus_Implementation() const override;
	//~ End IPDCombatInterface

	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void SetEnemyState(EPDEnemyState NewState);

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE EPDEnemyState GetEnemyState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE UAIPerceptionStimuliSourceComponent* GetStimuliSource() const { return StimuliSource; }

protected:
	/** 본 적이 속한 팀. 디자이너가 BP 디폴트에서 설정 (예: 2 = Hostile). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|AI")
	uint8 TeamID = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI")
	EPDEnemyState CurrentState = EPDEnemyState::Idle;

	/** 다른 AI가 본 적을 자극(시각/청각)으로 인지할 수 있도록 등록. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

	/** 디자이너 확장 hook. 상태 전환 직후 호출. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnEnemyStateChanged(EPDEnemyState NewState);

	/** 상태별 진입 hook. 자식 클래스에서 오버라이드 가능. */
	virtual void OnEnterState_Idle()   {}
	virtual void OnEnterState_Alert()  {}
	virtual void OnEnterState_Chase()  {}
	virtual void OnEnterState_Combat() {}
	virtual void OnEnterState_Dead();

	virtual void HandleDeath(AActor* Killer) override;
};
