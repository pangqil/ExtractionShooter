#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDDetectable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDDetectable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDDetectable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="PD|Vision")
	void OnEnterVision(AActor* Observer);

	UFUNCTION(BlueprintNativeEvent, Category="PD|Vision")
	void OnExitVision(AActor* Observer);
};