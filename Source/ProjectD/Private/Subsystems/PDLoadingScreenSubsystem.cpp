// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PDLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "DeveloperSettings/PDLoadingScreenSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Loading/PDLoadingScreenWidget.h"

void UPDLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(
		this, &UPDLoadingScreenSubsystem::HandlePreLoadMapWithContext);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UPDLoadingScreenSubsystem::HandlePostLoadMapWithWorld);
}

void UPDLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	HideLoadingScreen();

	Super::Deinitialize();
}

void UPDLoadingScreenSubsystem::Tick(float DeltaTime)
{
	if (!bHolding)
	{
		return;
	}

	HoldElapsed += DeltaTime;

	const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
	const float HoldSeconds = Settings ? Settings->HoldLoadingScreenExtraSeconds : 0.f;

	if (HoldElapsed >= HoldSeconds)
	{
		bHolding = false;
		HoldElapsed = 0.f;
		HideLoadingScreen();
	}
}

TStatId UPDLoadingScreenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPDLoadingScreenSubsystem, STATGROUP_Tickables);
}

void UPDLoadingScreenSubsystem::SetLoadingReason(const FText& InReason)
{
	CurrentReason = InReason;
	OnLoadingReasonUpdated.Broadcast(InReason);
}

void UPDLoadingScreenSubsystem::HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName)
{
	if (!ShouldDisplayInCurrentEnvironment())
	{
		return;
	}
	ShowLoadingScreen();
}

void UPDLoadingScreenSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!ActiveWidget)
	{
		return;
	}
	
	bHolding = true;
	HoldElapsed = 0.f;
}

void UPDLoadingScreenSubsystem::ShowLoadingScreen()
{
	if (ActiveWidget)
	{
		return;
	}

	const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
	if (!Settings || Settings->SoftLoadingScreenWidgetClass.IsNull())
	{
		return;
	}

	UClass* WidgetClass = Settings->SoftLoadingScreenWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UGameViewportClient* Viewport = GI->GetGameViewportClient();
	if (!Viewport) return;

	UUserWidget* Widget = CreateWidget<UUserWidget>(GI, WidgetClass);
	if (!Widget) return;

	ActiveWidget = Cast<UPDLoadingScreenWidget>(Widget);

	Viewport->AddViewportWidgetContent(Widget->TakeWidget(), 1000);
	Viewport->bDisableWorldRendering = true;

	if (ActiveWidget)
	{
		ActiveWidget->HandleLoadingReasonUpdated(CurrentReason);
		OnLoadingReasonUpdated.AddDynamic(ActiveWidget, &UPDLoadingScreenWidget::HandleLoadingReasonUpdated);
	}
}

void UPDLoadingScreenSubsystem::HideLoadingScreen()
{
	if (!ActiveWidget)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UGameViewportClient* Viewport = GI ? GI->GetGameViewportClient() : nullptr;

	if (Viewport)
	{
		Viewport->RemoveViewportWidgetContent(ActiveWidget->TakeWidget());
		Viewport->bDisableWorldRendering = false;
	}

	OnLoadingReasonUpdated.RemoveDynamic(ActiveWidget, &UPDLoadingScreenWidget::HandleLoadingReasonUpdated);

	ActiveWidget = nullptr;
}

bool UPDLoadingScreenSubsystem::ShouldDisplayInCurrentEnvironment() const
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		const UPDLoadingScreenSettings* Settings = GetDefault<UPDLoadingScreenSettings>();
		return Settings && Settings->bShouldShowLoadingScreenInEditor;
	}
#endif
	return true;
}