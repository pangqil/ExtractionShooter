#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDEquipmentModificationActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDEquipmentModificationActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDEquipmentModificationActor();

	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification")
	TObjectPtr<UStaticMeshComponent> WorkbenchMesh;
};
