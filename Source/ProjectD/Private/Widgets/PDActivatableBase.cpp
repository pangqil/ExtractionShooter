// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDActivatableBase.h"

#include "GameFramework/PlayerController.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

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

void UPDActivatableBase::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::OnFocusLost(InFocusEvent);

	if (!bLightDismissable) return;

	UPDFrontendUISubsystem* Subsystem = GetGameInstance()->GetSubsystem<UPDFrontendUISubsystem>();
	if (!Subsystem) return;

	// 자기 자신이 top인 레이어를 찾아서 pop. 어느 레이어에도 top이 아니면 무시
	static constexpr EUILayer AllLayers[] = { EUILayer::Frontend, EUILayer::GameMenu, EUILayer::Modal };
	for (EUILayer Layer : AllLayers)
	{
		if (Subsystem->GetTopOfLayer(Layer) == this)
		{
			Subsystem->PopFromLayer(Layer);
			return;
		}
	}
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
			break;
		}
	}
}
