// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDActivatableBase.h"

#include "GameFramework/PlayerController.h"

void UPDActivatableBase::Activate()
{
	if (bActivated) return;
	bActivated = true;

	ApplyInputMode();
	NativeOnActivated();
	OnActivated();
}

void UPDActivatableBase::Deactivate()
{
	if (!bActivated) return;
	bActivated = false;

	NativeOnDeactivated();
	OnDeactivated();
}

UWidget* UPDActivatableBase::GetDesiredFocusTarget_Implementation() const
{
	return nullptr;
}

void UPDActivatableBase::NativeOnActivated()
{
}

void UPDActivatableBase::NativeOnDeactivated()
{
}

void UPDActivatableBase::ApplyInputMode()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	switch (InputMode)
	{
	case EWidgetInputMode::Game:
		{
			FInputModeGameOnly Mode;
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(false);
			break;
		}
	case EWidgetInputMode::GameAndMenu:
		{
			FInputModeGameAndUI Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(bShowMouseCursor);
			break;
		}
	case EWidgetInputMode::Menu:
		{
			FInputModeUIOnly Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(bShowMouseCursor);
			break;
		}
	case EWidgetInputMode::Passive:
		{
			// 입력/커서 상태를 유지
			break;
		}
	}
}
