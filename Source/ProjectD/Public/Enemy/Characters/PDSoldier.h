#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDBipedEnemy.h"
#include "PDSoldier.generated.h"

class APDProjectile;

/**
 * 일반 보병 적 (Soldier).
 *  - APDBipedEnemy 의 가장 단순한 구체 클래스.
 *  - CombatComponent.OnAttackRequested 에 자동 구독 — Projectile 을 머즐 위치에서 스폰.
 *
 * Senior 관점:
 *  - "발사" 책임을 본 캐릭터가 직접 가지므로 BP 측은 모션/SFX/카메라 셰이크만 처리.
 *  - 본 클래스는 "디폴트" 발사 헬퍼만 제공 — 향후 무기 액터(APDWeaponBase) 시스템 도입 시
 *    bAutoFireProjectile=false 로 끄고 무기 측에서 처리할 수 있도록 후크.
 *
 * Mid 관점:
 *  - ProjectileClass / MuzzleSocketName / Damage 등은 BP 디폴트에서 데이터 주도 설정.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDSoldier : public APDBipedEnemy
{
	GENERATED_BODY()

public:
	APDSoldier();

	/** 내장 Projectile 자동 발사 토글. 외부 Weapon 컴포넌트가 발사를 인계할 때 false 로 끔. */
	UFUNCTION(BlueprintCallable, Category = "PD|Soldier|Weapon")
	void SetAutoFireProjectile(bool bEnabled) { bAutoFireProjectile = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "PD|Soldier|Weapon")
	bool IsAutoFireProjectileEnabled() const { return bAutoFireProjectile; }

protected:
	virtual void BeginPlay() override;

	/** 발사할 Projectile 클래스. BP_Projectile 등을 디자이너가 BP 디폴트에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	TSubclassOf<APDProjectile> ProjectileClass;

	/** Projectile 스폰 위치로 사용할 Mesh 소켓. 없으면 MuzzleOffset 으로 fallback. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Mesh 소켓이 없을 경우 캐릭터 ActorRotation 기준 머즐 오프셋(Local). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	FVector MuzzleOffset = FVector(60.f, 0.f, 60.f);

	/** Projectile 데미지. APDProjectile::InitProjectile 으로 주입. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon", meta = (ClampMin = "0.0"))
	float ProjectileDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bProjectileCanPenetrate = false;

	/**
	 * true: OnAttackRequested 에서 본 클래스가 자동으로 Projectile 을 스폰.
	 * false: BP / 무기 컴포넌트가 직접 OnAttackRequested 를 처리.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Soldier|Weapon")
	bool bAutoFireProjectile = true;

private:
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);

	void FireProjectile(AActor* Target);
};
