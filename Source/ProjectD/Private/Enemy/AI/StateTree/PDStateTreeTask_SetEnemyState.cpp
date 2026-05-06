#include "Enemy/AI/StateTree/PDStateTreeTask_SetEnemyState.h"

#include "AIController.h"
#include "Characters/Base/PDEnemyBase.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FPDStateTreeTask_SetEnemyState::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	APDEnemyBase* Enemy = Instance.AIController ? Cast<APDEnemyBase>(Instance.AIController->GetPawn()) : nullptr;
	if (!Enemy)
	{
		// Mid: ContextActor 미바인딩 또는 Pawn 미possess — 디자이너 셋업 오류로 즉시 실패.
		return EStateTreeRunStatus::Failed;
	}

	Enemy->SetEnemyState(Instance.NewState);
	return EStateTreeRunStatus::Running;
}
