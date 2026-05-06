// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDActivatableBase.h"

TOptional<FUIInputConfig> UPDActivatableBase::GetDesiredInputConfig() const
{
	if (InputMode == EWidgetInputMode::GameAndMenu)
	{
		return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::CapturePermanently);
	}
	if (InputMode == EWidgetInputMode::Menu)
	{
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);

	}
	if (InputMode == EWidgetInputMode::Game)
	{
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
	}
	
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
