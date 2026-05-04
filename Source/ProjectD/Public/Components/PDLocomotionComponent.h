#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDLocomotionComponent.generated.h"

class UCharacterMovementComponent;

UENUM(BlueprintType)
enum class EPDGait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run"),
	Sneak UMETA(DisplayName = "Sneak")
};

UENUM(BlueprintType)
enum class EPDAimState : uint8
{
    Hip     UMETA(DisplayName="Hip Fire"),
    Aim     UMETA(DisplayName="Aim Down Sights")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGaitChanged, EPDGait, NewGait);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimStateChanged, EPDAimState, NewAimState);

UCLASS(ClassGroup=(PD), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDLocomotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPDLocomotionComponent();
    
    UFUNCTION(BlueprintPure, Category="Locomotion")
    EPDGait GetCurrentGait() const { return CurrentGait; }
    
    UFUNCTION(BlueprintPure, Category="Locomotion")
    EPDAimState GetCurrentAimState() const { return CurrentAimState; }
    
    
    UFUNCTION(BlueprintCallable, Category="Locomotion")
    void SetGait(EPDGait NewGait);
    
    UFUNCTION(BlueprintCallable, Category="Locomotion")
    void SetAimState(EPDAimState NewState);

    UFUNCTION(BlueprintCallable, Category="Locomotion")
    void StartRun();

    UFUNCTION(BlueprintCallable, Category="Locomotion")
    void StopRun();
    
    
    UPROPERTY(BlueprintAssignable, Category="Locomotion")
    FOnGaitChanged OnGaitChanged;

    UPROPERTY(BlueprintAssignable, Category="Locomotion")
    FOnAimStateChanged OnAimStateChanged;

protected:
    virtual void BeginPlay() override;

    //--------------설정-------------------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Speed")
    float WalkSpeed = 400.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Speed")
    float RunSpeed = 700.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Speed")
    float SneakSpeed = 180.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion|Speed",
              meta=(ClampMin="0.0", ClampMax="1.0"))
    float AimSpeedMultiplier = 0.8f;

private:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Locomotion",
              meta=(AllowPrivateAccess="true"))
    EPDGait CurrentGait = EPDGait::Walk;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Locomotion",
              meta=(AllowPrivateAccess="true"))
    EPDAimState CurrentAimState = EPDAimState::Hip;

    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> CachedMovement;

    void UpdateMaxWalkSpeed();
    float ComputeTargetSpeed() const;
};

class PDLocomotionComponent
{
public:
	
	
};
