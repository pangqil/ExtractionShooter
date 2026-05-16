#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDRaidEntryPortal.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDRaidEntryPortal : public AActor
{
	GENERATED_BODY()

public:
	APDRaidEntryPortal();

protected:
	UPROPERTY(VisibleAnywhere, Category="PD|Portal")
	TObjectPtr<UBoxComponent> OverlapVolume;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Portal")
	FName RaidLevelName = TEXT("RaidMap");

	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Portal")
	void OnRaidEntryTriggered(APlayerController* PC);

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
	
	void EnterRaid(APlayerController* PC);
};
