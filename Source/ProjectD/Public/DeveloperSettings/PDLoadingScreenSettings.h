// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDLoadingScreenSettings.generated.h"

class UUserWidget;


UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Loading Screen"))
class PROJECTD_API UPDLoadingScreenSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (MetaClass = "/Script/UMG.UserWidget"))
	TSoftClassPtr<UUserWidget> SoftLoadingScreenWidgetClass;
	
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	float HoldLoadingScreenExtraSeconds = 3.f;
	
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	bool bShouldShowLoadingScreenInEditor = false;
};