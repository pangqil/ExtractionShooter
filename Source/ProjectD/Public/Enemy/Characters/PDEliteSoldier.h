#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "PDEliteSoldier.generated.h"

class APDCoverBase;

/**
 * 엘리트 솔저.
 *  - APDSoldier 상속하되 부모의 자동 풀오토 발사는 비활성(bAutoFireOnAttackRequested=false).
 *  - 발사 타이밍은 전적으로 BT 가 SetPeeking(true/false) 토글로 제어 — 피크 시에만 사격.
 *  - 커버 점유 라이프사이클(CurrentCover) 을 본 클래스가 단일 진실 원천으로 보관.
 *  - 수류탄 본체 구현은 별도(BP/외부) — 본 클래스는 ThrowGrenadeAt 훅과 GrenadeClass 슬롯만 노출.
 *
 * BT 가 호출하는 API:
 *  - SetInCover(Cover) / SetInCover(nullptr)  — 점유 진입/해제 + 피크 강제 종료.
 *  - SetPeeking(true) / SetPeeking(false)     — 발사 루프 on/off + BP 애니메이션 훅.
 *  - ThrowGrenadeAt(Location)                 — BlueprintNativeEvent. 기본 구현은 GrenadeClass 가 있으면 단순 스폰.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDEliteSoldier : public APDSoldier
{
	GENERATED_BODY()

public:
	APDEliteSoldier();

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE APDCoverBase* GetCurrentCover() const { return CurrentCover.Get(); }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsInCover() const { return bIsInCover; }

	UFUNCTION(BlueprintPure, Category = "PD|Elite|Cover")
	FORCEINLINE bool IsPeeking() const { return bIsPeeking; }

	/** nullptr 전달 시 현재 cover 해제. 같은 cover 재호출은 no-op. 피크 중이었다면 자동 종료. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetInCover(APDCoverBase* NewCover);

	/** true → 발사 루프 시작 + BP_OnStartPeek. false → 정지 + BP_OnEndPeek. */
	UFUNCTION(BlueprintCallable, Category = "PD|Elite|Cover")
	void SetPeeking(bool bPeek);

	/** 수류탄 투척 훅. 기본 구현은 GrenadeClass 미지정 시 경고, 지정 시 소켓에서 단순 스폰.
	 *  BP/자식 C++ 에서 오버라이드해 궤적/애니메이션/폭발을 구현. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Elite|Grenade")
	void ThrowGrenadeAt(const FVector& TargetLocation);

protected:
	virtual void OnEnterState_Dead() override;

	/** 수류탄 본체 클래스. 사용자가 BP 측에서 지정. nullptr 이면 기본 구현은 경고만. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Grenade")
	TSubclassOf<AActor> GrenadeClass;

	/** 수류탄 스폰 소켓. 캐릭터 메시(Mesh) 에 동명 소켓이 있어야 함. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Elite|Grenade")
	FName GrenadeSpawnSocketName = TEXT("GrenadeSocket");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsInCover = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Elite|Cover")
	bool bIsPeeking = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<APDCoverBase> CurrentCover;

	// 디자이너용 애니메이션/VFX 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEnterCover(APDCoverBase* Cover);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnExitCover();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnStartPeek();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Elite|Cover")
	void BP_OnEndPeek();
};
