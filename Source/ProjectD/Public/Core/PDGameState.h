// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PDGameState.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTD_API APDGameState : public AGameState
{
	GENERATED_BODY()

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
};
