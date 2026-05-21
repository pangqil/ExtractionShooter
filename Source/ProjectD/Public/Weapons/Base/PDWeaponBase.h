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
class USoundBase;
class UGCN_Weapon_Equip;
class UGCN_Weapon_Swing;
class UGCN_Weapon_MeleeHit;

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USphereComponent> PickupCollision;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FGameplayTag WeaponTypeTag;


	UPROPERTY(ReplicatedUsing=OnRep_WeaponIdentity, EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FName ItemID;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Animation")
	FName LeftHandGripSocket = TEXT("LeftHandGrip");


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Stats")
	TArray<FWeaponLevelStats> LevelStats;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> EquipSound;


	UPROPERTY(ReplicatedUsing=OnRep_WeaponIdentity, EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	EWeaponType WeaponType = EWeaponType::None;


	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	int32 CurrentLevel = 1;

	UPROPERTY(ReplicatedUsing=OnRep_Dropped, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	bool bIsDropped = false;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponOwner, VisibleAnywhere, BlueprintReadOnly, Category="Weapon|State")
	TObjectPtr<AActor> WeaponOwner;

	UFUNCTION()
	void OnRep_Dropped();

	UFUNCTION()
	void OnRep_WeaponOwner();

	UFUNCTION()
	void OnRep_WeaponIdentity();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPickedUp();

	void ApplyReplicatedWeaponOwner();
	void ApplyPickedUpPresentation();

public:
	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponLevelChanged OnWeaponLevelChanged;

	UFUNCTION(BlueprintImplementableEvent, Category="Weapon")
	void OnPickupFailed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void OnEquip(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon")
	void OnUnequip();

	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void UpgradeLevel();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetLevel(int32 NewLevel);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void SetDropped(bool bDropped);

	UFUNCTION(BlueprintPure, Category="Weapon")
	const FWeaponLevelStats& GetCurrentStats() const;

	FORCEINLINE int32              GetCurrentLevel()        const { return CurrentLevel; }
	FORCEINLINE EWeaponType        GetWeaponType()          const { return WeaponType; }
	FORCEINLINE AActor*            GetWeaponOwner()         const { return WeaponOwner.Get(); }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh()     const { return WeaponMesh; }
	FORCEINLINE FGameplayTag       GetWeaponTypeTag()       const { return WeaponTypeTag; }
	FORCEINLINE TSubclassOf<UAnimInstance> GetWeaponAnimLayerClass() const { return WeaponAnimLayerClass; }
	FORCEINLINE FName              GetLeftHandGripSocket()  const { return LeftHandGripSocket; }
	FORCEINLINE FName              GetItemID()              const { return ItemID; }

protected:
	FVector GetAimDirectionFromOwner(const FVector& StartLocation) const;

	friend class UGCN_Weapon_Equip;
	friend class UGCN_Weapon_Swing;
	friend class UGCN_Weapon_MeleeHit;
};
