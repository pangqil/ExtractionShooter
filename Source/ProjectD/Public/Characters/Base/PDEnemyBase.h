#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "Interfaces/PDDetectable.h"
#include "GenericTeamAgentInterface.h"
#include "PDEnemyBase.generated.h"

class UAIPerceptionStimuliSourceComponent;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase, public IPDCombatInterface, public IPDDetectable, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	APDEnemyBase();

	//~ Begin IPDCombatInterface
	virtual uint8 GetTeamID_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;
	virtual EPDBatteryStatus GetBatteryStatus_Implementation() const override;
	//~ End IPDCombatInterface

	// 엔진 perception 의 affiliation 시스템에 같은 TeamID 를 노출.
	// AAIController 가 possess 한 폰의 인터페이스로 자동 위임하므로 controller 측 별도 작업 불필요.
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamID); }

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
	virtual void OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure) override;
};
