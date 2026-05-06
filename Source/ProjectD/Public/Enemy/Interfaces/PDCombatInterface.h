#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDCombatInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 적/플레이어 구분, 생사, 배터리 상태를 외부(StateTree, Combat 시스템 등)에서
 * 조회하기 위한 경량 인터페이스.
 *
 * Senior 관점: 구체 클래스(APDEnemyBase, ABipedEnemy)에 직접 의존하지 않고
 * 인터페이스만 알면 되도록 분리하여 결합도를 낮춤. 추후 Player 측이 같은
 * 인터페이스를 구현해도 동일 시스템에서 식별 가능.
 *
 * Mid 관점: 모든 메서드는 BlueprintNativeEvent로 노출하여 BP 서브클래스도
 * 오버라이드 가능. const 한정자 부여로 호출자 측에서 안전하게 호출.
 *
 * 주의(UE 5.5+): UInterface 의 BlueprintNativeEvent 는 UHT 가 GENERATED_BODY()
 * 내부에 _Implementation 의 기본 본문(return TReturnType{};) 을 자동 생성한다.
 * 따라서 .cpp 에 _Implementation 본문을 다시 정의하면 중복 정의 오류(C2084).
 * 비-기본값 디폴트가 필요하면 BlueprintImplementableEvent 로 바꾸거나,
 * 구현 클래스(APDEnemyBase 등)에서 오버라이드한다.
 */
class PROJECTD_API IPDCombatInterface
{
	GENERATED_BODY()

public:
	/** 팀 ID. 0=Neutral, 1=Player, 2=EnemyA, ... 0이 기본값. */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Combat")
	uint8 GetTeamID() const;

	/** 살아있는지 여부. 기본값 false (반드시 오버라이드). */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Combat")
	bool IsAlive() const;

	/** Battery 상태. Biped가 아니면 None. */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Combat")
	EPDBatteryStatus GetBatteryStatus() const;
};
