// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "TimerManager.h"
#include "PDMeleeWeapon.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMeleeWeapon : public APDWeaponBase
{
	GENERATED_BODY()
	
public:
	APDMeleeWeapon();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Melee")
	TObjectPtr<UBoxComponent> AttackCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Melee")
	float AttackDamage = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Melee")
	float AttackCooldown = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Melee")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY()
	TSet<AActor*> AlreadyHitActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon|Melee")
	bool bCanFire = true;

	FTimerHandle FireCooldownHandle;

public:
	virtual void Fire_Implementation() override;
	virtual void Reload_Implementation() override {}

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Melee")
	void EnableAttackCollision();

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Melee")
	void DisableAttackCollision();

protected:
	UFUNCTION()
	void OnAttackOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void ApplyDamage(AActor* TargetActor, float DamageAmount);
};