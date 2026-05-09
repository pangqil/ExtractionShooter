// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDEnemyBase.h"
#include "PDBaseNoiseEmitter.generated.h"

class UStateTreeComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDBaseNoiseEmitter : public APDEnemyBase
{
	GENERATED_BODY()

public:
	APDBaseNoiseEmitter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|NoiseGeneration")
	float MaxNoiseRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|NoiseGeneration")
	float NoiseRangeRandomDeviation = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|NoiseGeneration")
	FName NoiseTag = "EmitterNoise";

	UFUNCTION(BlueprintCallable, Category = "PD|NoiseGeneration")
	void EmitNoise();

protected:
	virtual void BeginPlay() override;


	/** BP 디자이너가 자체 의사결정(정기 emit 등) 작성용. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTreeComponent;
};
