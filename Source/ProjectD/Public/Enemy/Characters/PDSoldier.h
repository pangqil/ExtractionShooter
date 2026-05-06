#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "PDSoldier.generated.h"

/**
 * 일반 보병 적 (Soldier).
 *  - APDBipedEnemy 의 가장 단순한 구체 클래스.
 *  - 디자이너가 BP_Soldier 등에서 메시/애님/무기/StateTree 를 지정.
 *
 * Senior 관점: C++ 레벨에서는 의도적으로 비워둠 — 디자이너 BP에서 데이터 주도(Data-driven)로
 *              밸런싱하도록 함. 특수 거동(투척병/저격병 등)은 별도 자식 클래스로 추가.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDSoldier : public APDBipedEnemy
{
	GENERATED_BODY()

public:
	APDSoldier();
};
