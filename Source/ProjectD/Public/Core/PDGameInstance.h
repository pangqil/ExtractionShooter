// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Type/Types.h"
#include "PDGameInstance.generated.h"

UCLASS()
class PROJECTD_API UPDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Data")
	void SavePlayerData(const FPDPlayerData& InData);

	UFUNCTION(BlueprintCallable, Category = "PD|Data")
	FPDPlayerData LoadPlayerData() const;

protected:
	UPROPERTY()
	FPDPlayerData PlayerData;
};
