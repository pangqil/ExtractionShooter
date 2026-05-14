#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "Interfaces/PDDetectable.h"
#include "Type/Types.h"
#include "PDEnemyBase.generated.h"

class APDItemBase;
class APDWeaponBase;
class APDStashActor;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase, public IPDCombatInterface, public IPDDetectable
{
	GENERATED_BODY()

public:
	APDEnemyBase();

	//~ Begin IPDCombatInterface
	virtual uint8 GetTeamID_Implementation() const override;
	virtual EPDStaminaStatus GetStaminaStatus_Implementation() const override;
	//~ End IPDCombatInterface

	// Combat이 아닐 때 피격되면 공격자 위치를 NoiseHint로 등록해 BT가 그 방향으로 추적하게 한다.
	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;

	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void SetEnemyState(EPDEnemyState NewState);

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE EPDEnemyState GetEnemyState() const { return CurrentState; }

	// 조준 타겟 설정 (BT Task에서 SetAimTarget 호출)
	UFUNCTION(BlueprintCallable, Category = "PD|AI|Weapon")
	void SetAimTarget(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "PD|AI|Weapon")
	void ClearAimTarget();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI")
	EPDEnemyState CurrentState = EPDEnemyState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Quest")
	FName QuestEnemyID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Quest")
	bool bIsQuestEnemy = false;

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

	// ─── 사망 드랍 ─────────────────────────────────────────────
	/** 사망 시 굴릴 드랍 테이블. 디자이너 BP 디폴트에서 채움. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TArray<FPDLootEntry> LootTable;

	/** 시체/박스 등 상호작용 가능한 컨테이너. 비어있으면 시체 미스폰. APDStashActor 자식이면 자동으로 슬롯 주입. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TSubclassOf<AActor> CorpseContainerClass;

	/** 드랍 위치 변동 반경 — 0 이면 정확히 사망 위치에 스폰. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot", meta = (ClampMin = "0.0"))
	float LootSpawnRadius = 50.f;

	/** Loot box 컨테이너에 채워질 소비 아이템 풀. Enemy 별로 BP에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TArray<FPDConsumableDropEntry> ConsumableDropTable;

	/** true면 HandleDeath 완료 직후 본 액터를 소멸(시체 미존속). 플레이어 캐릭터 등 자체 사망 처리에서는 자식이 false로. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	bool bDestroyOnDeath = true;

	/** LootTable을 굴려 아이템을 스폰. 호출 시점은 자식이 결정 가능 (기본은 OnEnterState_Dead). */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	virtual void DropLootOnDeath();

	/** 시체/박스 컨테이너 스폰. 비어있는 클래스면 즉시 return. Stash 컨테이너면 무기 + 소비 아이템 슬롯을 주입. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	virtual void SpawnCorpseContainer();

	/** 자식 클래스가 장착 무기를 슬롯으로 변환해 채워넣는 hook. 기본 구현은 no-op. */
	virtual void HarvestEquippedWeaponSlots(TArray<FPDInventorySlot>& OutSlots) const {}

	/** ConsumableDropTable을 굴려 통과한 행을 슬롯으로 변환해 채운다. */
	void BuildConsumableLootSlots(TArray<FPDInventorySlot>& OutSlots) const;

	/** 디자이너가 BP에서 드랍 후 처리(VFX/사운드 등) 작성 가능. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Loot")
	void OnLootDropped(const TArray<AActor*>& SpawnedItems);

private:
	// CombatComponent::OnTargetChanged → WeaponComponent::SetAimTarget 자동 연결
	UFUNCTION()
	void OnCombatTargetChanged(AActor* NewTarget);
};
