// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "BaseWeaponComponent.generated.h"

class APDProjectile;

/**
 * 프로토타입 데모용 무기 컴포넌트.
 *  - StaticMesh(라이플) 시각 + Soldier 손 소켓 부착 + Projectile 발사.
 *  - BeginPlay 에서 Owner 의 CombatComponent.OnAttackRequested 에 자동 구독 →
 *    StateTree 의 FireAtTarget 가 작동하면 본 컴포넌트가 실제 발사를 담당.
 *  - 본 컴포넌트가 존재하면 APDSoldier 의 내장 자동 발사는 비활성 (책임 인계).
 *
 * ⚠️ 주의: 추후 Player/Weapon 정식 시스템(APDWeaponBase 등) 도입 시 본 컴포넌트는 폐기 예정.
 *          따라서 의존성을 최소화 — Soldier 의 SetAutoFireProjectile 한 곳만 호출.
 */
UCLASS(Blueprintable, ClassGroup = "PD", meta = (BlueprintSpawnableComponent))
class PROJECTD_API UBaseWeaponComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UBaseWeaponComponent();

	/** 타겟을 향해 발사. CombatComponent.OnAttackRequested 가 자동 호출. */
	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	virtual void Shoot(AActor* Target);

	/** 발사할 Projectile 클래스. 강타입으로 두어 InitProjectile(Damage/Owner) 가 보장됨. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Bullet")
	TSubclassOf<APDProjectile> BulletClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Bullet", meta = (ClampMin = "0.0"))
	float BulletDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Bullet")
	bool bBulletCanPenetrate = false;

	/** 무기 메시 자체의 머즐 소켓. 없으면 MuzzleOffset 으로 fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Muzzle")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** 머즐 소켓이 없을 때 컴포넌트(=무기) 로컬 기준 오프셋. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Muzzle")
	FVector MuzzleOffset = FVector(150.f, 0.f, 0.f);

	/** Soldier 메시의 어느 본/소켓에 부착할지. (USceneComponent::AttachSocketName 과 이름 충돌 회피) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Weapon|Attach")
	FName WeaponAttachSocket = TEXT("hand_right");

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleAttackRequested(AActor* Target);
};
