#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

/**
 * Enemy FSM 상태.
 * - Idle    : 기본/순찰 + 중얼거림(Audio/UI hook)
 * - Alert   : 소리/의심 자극 → 위치 조사
 * - Chase   : 타겟 시각 확인 → 추격 (시야 상실 시 마지막 위치로 이동)
 * - Combat  : 지속적 시야 또는 피격 → 공격 + 주변 동료에게 알림
 * - Dead    : HP <= 0 → 애니메이션, 충돌 비활성, 루트 생성
 *
 * Senior 관점: 상태값을 uint8 기반 UENUM으로 두면 BlackboardKey/StateTree 비교가
 * O(1)이고, replication 비용도 1바이트로 최소화됨.
 */
UENUM(BlueprintType)
enum class EPDEnemyState : uint8
{
	Idle    UMETA(DisplayName = "Idle"),
	Alert   UMETA(DisplayName = "Alert"),
	Chase   UMETA(DisplayName = "Chase"),
	Combat  UMETA(DisplayName = "Combat"),
	Dead    UMETA(DisplayName = "Dead"),
};

/**
 * Battery(스태미너) 상태.
 * Biped(이족 보행 적)에게만 의미가 있으며 그 외에는 None.
 *
 * Mid 관점: 임계값을 enum으로 추상화하면 디자이너/StateTree가 정확한
 * float 값을 알 필요 없이 의사결정 가능 → 밸런스 변경 시 컴포넌트 임계값만 수정.
 */
UENUM(BlueprintType)
enum class EPDBatteryStatus : uint8
{
	None       UMETA(DisplayName = "None"),       // 비-Biped 또는 Battery 미사용
	Full       UMETA(DisplayName = "Full"),       // > 80%
	Optimal    UMETA(DisplayName = "Optimal"),    // 50% ~ 80%
	Low        UMETA(DisplayName = "Low"),        // 20% ~ 50%
	Exhausted  UMETA(DisplayName = "Exhausted"),  // < 20%
};
