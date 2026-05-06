// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDAttributeBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPDAttributeBarWidget::SetValues(float Current, float Max)
{
	const float Percent = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;

	if (ProgressBar)
	{
		ProgressBar->SetPercent(Percent);
	}

	if (Text_Value)
	{
		Text_Value->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"),
				FMath::RoundToInt(Current),
				FMath::RoundToInt(Max))));
	}
}
