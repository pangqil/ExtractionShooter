#include "Enemy/AI/StateTree/PDStateTreeTask_FireAtTarget.h"

#include "AIController.h"
#include "Enemy/AI/StateTree/PDStateTreeTaskCommon.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FPDStateTreeTask_FireAtTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat || !Combat->HasValidTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	// Mid: SetFocus 는 한 번만 호출 — AIController 가 캐릭터 회전을 자동 보정.
	if (AActor* Target = Combat->GetCurrentTarget())
	{
		Instance.AIController->SetFocus(Target);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPDStateTreeTask_FireAtTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPDCombatComponent* Combat = PDStateTreeUtil::GetCombat(Instance.AIController);
	if (!Combat || !Combat->HasValidTarget())
	{
		// 타겟이 사라졌으면 상위 분기에서 Idle/Chase 로 전이.
		return EStateTreeRunStatus::Failed;
	}

	// 쿨다운/사거리 검증은 CombatComponent 내부. 호출 빈도와 무관하게 안전.
	Combat->RequestAttack();
	return EStateTreeRunStatus::Running;
}

void FPDStateTreeTask_FireAtTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// Junior: Focus 를 풀어주지 않으면 다음 상태에서도 캐릭터가 계속 타겟을 바라봄.
	if (Instance.AIController)
	{
		Instance.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
