#pragma once

#include "CoreMinimal.h"
#include "Items/PDStashActor.h"
#include "PDLootBoxActor.generated.h"

/**
 * 적 사망 시 스폰되는 일회용 컨테이너.
 * Stash와 동일한 UI/Drag-Drop을 재활용하기 위해 APDStashActor 를 상속.
 * 현재는 확장 포인트만 열어둔 빈 클래스 — BP 변형(메시/사운드/자동소멸 등)으로 디자이너가 커스터마이즈.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDLootBoxActor : public APDStashActor
{
	GENERATED_BODY()

public:
	APDLootBoxActor();
};
