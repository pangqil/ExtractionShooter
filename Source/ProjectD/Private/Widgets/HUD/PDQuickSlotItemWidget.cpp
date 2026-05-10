// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/HUD/PDQuickSlotItemWidget.h"

void UPDQuickSlotItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CurrentScale = NormalScale;
	TargetScale = NormalScale;
	SetRenderScale(CurrentScale);
}

void UPDQuickSlotItemWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (CurrentScale.Equals(TargetScale, 0.001f))
	{
		return;
	}

	CurrentScale = FMath::Vector2DInterpTo(CurrentScale, TargetScale, InDeltaTime, ScaleInterpSpeed);
	SetRenderScale(CurrentScale);
}

void UPDQuickSlotItemWidget::SetSelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}

	bSelected = bInSelected;
	ApplySelectedState();
}

void UPDQuickSlotItemWidget::ApplySelectedState()
{
	if (SelectPulseAnimation)
	{
		if (bSelected)
		{
			PlayAnimation(SelectPulseAnimation, 0.f, 1, EUMGSequencePlayMode::Forward);
		}
		else
		{
			StopAnimation(SelectPulseAnimation);
		}
	}
	
	TargetScale = bSelected ? SelectedScale : NormalScale;
}
