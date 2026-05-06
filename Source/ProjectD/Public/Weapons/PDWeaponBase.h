#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDWeaponBase.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	APDWeaponBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon")
	TWeakObjectPtr<AActor> WeaponOwner;

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnEquip(AActor* NewOwner);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
    void OnUnequip();

    // 각 AnimNotify에서 직접 호출 — BP에서 추가 효과 오버라이드 가능
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void EjectShell();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void DropMagazine();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void AttachNewMagazine();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnBoltPulled();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnBoltReleased();

    // 샷건: 탄 한 발 약실에 넣기 완료
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon|Animation")
    void OnShellInserted();

    // IPDInteractable 구현
    // 플레이어가 F키 누르면 줍기
    virtual void Interact_Implementation(AActor* Interactor) override;

	FORCEINLINE AActor* GetWeaponOwner() const { return WeaponOwner.Get(); }
};
