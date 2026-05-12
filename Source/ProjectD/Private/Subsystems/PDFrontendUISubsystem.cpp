// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PDFrontendUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDRootLayout.h"
#include "Widgets/PDWidgetStack.h"

namespace
{
	constexpr EUILayer GAllLayers[] = { EUILayer::Frontend, EUILayer::GameMenu, EUILayer::Modal };
}

UPDFrontendUISubsystem* UPDFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		return UGameInstance::GetSubsystem<UPDFrontendUISubsystem>(World->GetGameInstance());
	}
	return nullptr;
}

void UPDFrontendUISubsystem::RegisterRootLayout(UPDRootLayout* InRootLayout)
{
	if (RootLayout == InRootLayout) return;

	// 기존 등록 해제
	if (RootLayout)
	{
		for (EUILayer Layer : GAllLayers)
		{
			if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
			{
				Stack->OnStackChanged.RemoveAll(this);
			}
		}
	}

	RootLayout = InRootLayout;

	// 신규 등록 및 델리게이트 구독
	if (RootLayout)
	{
		for (EUILayer Layer : GAllLayers)
		{
			if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
			{
				Stack->OnStackChanged.AddUObject(this, &UPDFrontendUISubsystem::HandleAnyStackChanged);
			}
		}
	}

	OnEffectiveUIStateChanged.Broadcast(ComputeEffectiveInputMode());
}

void UPDFrontendUISubsystem::UnregisterRootLayout()
{
	if (!RootLayout) return;

	for (EUILayer Layer : GAllLayers)
	{
		if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
		{
			Stack->OnStackChanged.RemoveAll(this);
		}
	}
	RootLayout = nullptr;
}

UPDActivatableBase* UPDFrontendUISubsystem::PushToLayer(EUILayer Layer, TSubclassOf<UPDActivatableBase> ScreenClass)
{
	if (!RootLayout)
	{
		UE_LOG(LogTemp, Warning, TEXT("PushToLayer called but RootLayout is not registered. Phase 2에서 PC가 등록합니다."));
		return nullptr;
	}
	UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer);
	return Stack ? Stack->Push(ScreenClass) : nullptr;
}

void UPDFrontendUISubsystem::PopFromLayer(EUILayer Layer)
{
	if (!RootLayout) return;
	if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
	{
		Stack->Pop();
	}
}

void UPDFrontendUISubsystem::ClearLayer(EUILayer Layer)
{
	if (!RootLayout) return;
	if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
	{
		Stack->Clear();
	}
}

UPDActivatableBase* UPDFrontendUISubsystem::GetTopOfLayer(EUILayer Layer) const
{
	if (!RootLayout) return nullptr;
	UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer);
	return Stack ? Stack->GetTop() : nullptr;
}

void UPDFrontendUISubsystem::HandleAnyStackChanged(UPDWidgetStack* /*ChangedStack*/)
{
	OnEffectiveUIStateChanged.Broadcast(ComputeEffectiveInputMode());
}

EWidgetInputMode UPDFrontendUISubsystem::ComputeEffectiveInputMode() const
{
	if (!RootLayout) return EWidgetInputMode::Game;

	// 우선순위(=z-order top down): Modal > GameMenu > Frontend. 모두 비면 Game (인게임 상태).
	static constexpr EUILayer Priority[] = { EUILayer::Modal, EUILayer::GameMenu, EUILayer::Frontend };
	for (EUILayer Layer : Priority)
	{
		if (UPDWidgetStack* Stack = RootLayout->GetLayerStack(Layer))
		{
			if (UPDActivatableBase* Top = Stack->GetTop())
			{
				return Top->GetInputMode();
			}
		}
	}
	return EWidgetInputMode::Game;
}
