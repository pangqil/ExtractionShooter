// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDLoadingScreenSettings.generated.h"

class UUserWidget;
class UTexture2D;


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

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Splash")
	TArray<TSoftObjectPtr<UTexture2D>> SplashImagePool;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Splash")
	TSoftObjectPtr<UTexture2D> StaticSplashOverride;
};