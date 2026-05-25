#pragma once

#include "CoreMinimal.h"
#include "Items/Station/PDStationActorBase.h"
#include "PDStashActor.generated.h"

class APDStashActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDStashStateChangedSignature, APDStashActor*, StashActor);

class APDPlayerController;
class UPDStashComponent;
class USoundBase;

UCLASS(Blueprintable)
class PROJECTD_API APDStashActor : public APDStationActorBase
{
	GENERATED_BODY()

public:
	APDStashActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	FORCEINLINE UPDStashComponent* GetStashComponent() const { return StashComponent; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Stash|Events")
	FPDStashStateChangedSignature OnStorageOpened;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stash|Events")
	FPDStashStateChangedSignature OnStorageClosed;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UPDStashComponent> StashComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	FName DoorBoneName = TEXT("Door_Hinge_01");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	FVector DoorRotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	float ClosedDoorAngle = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	float OpenDoorAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door", meta = (ClampMin = "0.0"))
	float DoorInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound", meta = (ClampMin = "0.0"))
	float SoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound", meta = (ClampMin = "0.0"))
	float SoundPitchMultiplier = 1.f;

protected:
	// LootBox ???åÏÉù ?¥Îûò?§Í? ?êÏ≤¥ close ?∏Î¶¨Í≤åÏù¥??OnLootInterfaceClosed ????Î∞îÏù∏?©ÌïòÍ∏??ÑÌï¥ protected.
	void SetDoorOpen(bool bOpen, bool bInstant = false);

	/** Ïª®ÌÖå?¥ÎÑà close ?∏Î¶¨Í≤åÏù¥??Íµ¨ÎèÖ. Í∏∞Î≥∏ Íµ¨ÌòÑ?Ä OnStashInterfaceClosed. ?êÏãù?Ä ?§Î•∏ ?∏Î¶¨Í≤åÏù¥?∏Î°ú override. */
	virtual bool IsStationOpen(APDPlayerController* PlayerController) const override;
	virtual void OpenStation(APDPlayerController* PlayerController) override;
	virtual void CloseStation(APDPlayerController* PlayerController) override;
	virtual void BindStationClose(APDPlayerController* PlayerController) override;
	virtual void UnbindStationClose() override;
	virtual void HandleStationOpened(APDPlayerController* PlayerController) override;
	virtual void HandleStationClosed() override;

	/** close ?úÍ∑∏??Í≥µÌÜµ Ï≤òÎ¶¨ ???êÍ∏∞ Ïª¥Ìè¨?åÌä∏?Ä Îß§Ïπ≠?òÎ©¥ Î¨??´Í≥† unbind. ?úÍ∑∏?àÏ≤ò???ëÏ™Ω ?∏Î¶¨Í≤åÏù¥?∏Í? ?ôÏùº. */
	UFUNCTION()
	void HandleContainerClosed(UPDStashComponent* ClosedStashComponent);

private:
	void ApplyDoorAngle(float Angle);
	void PlayDoorSound(bool bOpen) const;

	float CurrentDoorAngle = 100.f;
	float TargetDoorAngle = 100.f;
};
