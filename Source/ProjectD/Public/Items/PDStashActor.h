#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDStashActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPDStashComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDStashActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDStashActor();

	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	FORCEINLINE UPDStashComponent* GetStashComponent() const { return StashComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UStaticMeshComponent> StashMesh;

	// 박스마다 독립 인벤토리. 격자 사이즈/내용물은 BP에서 인스턴스별로 설정 가능.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UPDStashComponent> StashComponent;
};
