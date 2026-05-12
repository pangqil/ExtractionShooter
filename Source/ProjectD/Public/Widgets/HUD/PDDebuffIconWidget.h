// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDDebuffIconWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDDebuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIconData(UTexture2D* IconTexture, FVector2D IconSize);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Background;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;
};
