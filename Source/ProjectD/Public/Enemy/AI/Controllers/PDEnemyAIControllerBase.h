#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PDEnemyAIControllerBase.generated.h"

class UStateTreeAIComponent;
class UPDPerceptionComponent;
class UStateTree;
struct FAIStimulus;

/**
 * Enemy AI 디버그 전용 로그 카테고리.
 *  - 콘솔: `Log LogPDAI Verbose` / `Log LogPDAI Off` 로 일괄 토글.
 *  - Senior 관점: AI 진입 체인(Possess → Perception → StateTree Task → MoveTo)을
 *    같은 채널로 묶어 정지 원인을 단계별로 좁힐 수 있게 함.
 */
PROJECTD_API DECLARE_LOG_CATEGORY_EXTERN(LogPDAI, Log, All);

/**
 * Enemy 전용 AIController 베이스.
 *
 * 구성:
 *  - UStateTreeAIComponent : FSM 의 실제 실행은 디자이너가 작성한 StateTree 에 위임.
 *  - UPDPerceptionComponent : 시각/청각 자극을 도메인 이벤트로 노출.
 *
 * Senior 관점: 본 클래스는 "AI 의 인프라 조립"만 담당 — 상태 결정 로직은 StateTree(디자이너)가,
 *              액션은 컴포넌트가 수행. C++ 가 BP/StateTree 를 거스르지 않도록 책임 경계가 명확함.
 *
 * Mid 관점: StartStateTree 는 OnPossess 후 호출 — Pawn 이 valid 한 상태에서 StateTree 가
 *           ContextActor 로 바인딩되도록 보장.
 */
UCLASS()
class PROJECTD_API APDEnemyAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	APDEnemyAIControllerBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/**
	 * 디버그 시각화 — 시야 콘, 청각 반경, 현재 타겟/NoiseHint, NavPath 를 매 틱 그림.
	 *
	 * 토글: 콘솔 `pd.ai.debugdraw 1` / `pd.ai.debugdraw 0`. shipping 빌드에서는 컴파일 제거.
	 *
	 * Senior 관점: 디버그 코드는 AIController 한 곳에 응집 — Perception/Combat/Path 가 모두
	 *              여기서 접근 가능해 단일 진단 시점 확보. CVar 가드로 런타임 비용 0.
	 */
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	UPDPerceptionComponent* GetPDPerception() const;

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE UStateTreeAIComponent* GetStateTreeAIComponent() const { return StateTreeAIComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	// 주의: PerceptionComponent 는 부모(AAIController)에 이미 UPROPERTY 로 선언되어 있어
	//       파생 클래스에서 같은 이름으로 다시 선언하면 UHT shadowing 오류가 발생.
	//       따라서 별도 멤버를 두지 않고, 생성자에서 부모의 PerceptionComponent 에
	//       UPDPerceptionComponent 를 CreateDefaultSubobject 로 주입한 뒤 GetPDPerception() 으로 캐스팅 접근.

private:
	/** Perception 도메인 이벤트 → AIController 측 처리 (BP 확장 hook 으로 broadcast). */
	UFUNCTION()
	void HandleTargetSpotted(AActor* Target);

	UFUNCTION()
	void HandleTargetLost(AActor* Target, FVector LastKnownLocation);

	UFUNCTION()
	void HandleNoiseHeard(AActor* NoiseInstigator, FVector Location);

protected:
	/** BP 확장 hook — StateTree 에서 직접 컴포넌트 이벤트 구독해도 되고, 본 hook 을 써도 됨. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnTargetSpotted(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnTargetLost(AActor* Target, FVector LastKnownLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnNoiseHeard(AActor* NoiseInstigator, FVector Location);

private:
	/** Tick 매 프레임 호출 — 시각/청각/타겟/Path 를 DrawDebug 로 시각화. */
	void DrawAIDebug() const;
};
