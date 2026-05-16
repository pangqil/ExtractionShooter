#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Interfaces/PDInteractable.h"
#include "PDWeaponBase.generated.h"

class APDWeaponBase;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponLevelChanged, APDWeaponBase*, Weapon, int32, NewLevel);

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()
	friend class APDPlayerCharacter;

public:
	APDWeaponBase();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Weapon|Component")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|Component")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Config")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Stats")
	TArray<FWeaponLevelStats> LevelStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|GAS")
	FGameplayTag WeaponTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Weapon|Animation")
	FName LeftHandGripSocket = TEXT("LeftHandGrip");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon|State")
	int32 CurrentLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Weapon|State")
	bool bIsDropped = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Weapon")
	TWeakObjectPtr<AActor> WeaponOwner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|Weapon|Item")
	FName ItemID;

public:
	UPROPERTY(BlueprintAssignable, Category="PD|Weapon|Events")
	FOnWeaponLevelChanged OnWeaponLevelChanged;

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Weapon")
	void OnPickupFailed();

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void OnEquip(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PD|Weapon")
	void OnUnequip();

	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, Category="PD|Weapon")
	void UpgradeLevel();

	UFUNCTION(BlueprintCallable, Category="PD|Weapon")
	void SetLevel(int32 NewLevel);

	UFUNCTION(BlueprintCallable, Category="PD|Weapon")
	void SetDropped(bool bDropped);

	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	const FWeaponLevelStats& GetCurrentStats() const;

	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	FORCEINLINE int32 GetCurrentLevel() const { return CurrentLevel; }

UFUNCTION(BlueprintPure, Category="PD|Weapon")
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	FORCEINLINE AActor* GetWeaponOwner() const { return WeaponOwner.Get(); }

	UFUNCTION(BlueprintPure, Category="PD|Weapon")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|GAS")
	FORCEINLINE FGameplayTag GetWeaponTypeTag() const { return WeaponTypeTag; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Animation")
	FORCEINLINE TSubclassOf<UAnimInstance> GetWeaponAnimLayerClass() const { return WeaponAnimLayerClass; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Animation")
	FORCEINLINE FName GetLeftHandGripSocket() const { return LeftHandGripSocket; }

	UFUNCTION(BlueprintPure, Category="PD|Weapon|Item")
	FORCEINLINE FName GetItemID() const { return ItemID; }

protected:
	FVector GetAimDirectionFromOwner(const FVector& StartLocation) const;
};
