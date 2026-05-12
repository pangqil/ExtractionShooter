// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDDebuffIconWidget.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UPDDebuffIconWidget::SetIconData(UTexture2D* IconTexture, FVector2D IconSize)
{
	if (!Image_Icon || !IconTexture) return;

	Image_Icon->SetBrushFromTexture(IconTexture);
	Image_Icon->SetDesiredSizeOverride(IconSize);
}
