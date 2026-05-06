// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/PDFrontendUISubsystem.h"

#include "Engine/AssetManager.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/PDActivatableBase.h"
#include "Widgets/PDPrimaryLayout.h"

UPDFrontendUISubsystem* UPDFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		// WorldContextObject로부터 월드 찾기. 찾지 못하면 크래시
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		
		return UGameInstance::GetSubsystem<UPDFrontendUISubsystem>(World->GetGameInstance());
	}
	
	return nullptr;
}


void UPDFrontendUISubsystem::RegisterCreatePrimaryLayoutWidget(UPDPrimaryLayout* CreatedWidget)
{	
	checkf(IsValid(CreatedWidget), TEXT("생성된 PrimaryWidget이 존재하지 않음"));
	CreatedPrimaryLayout = CreatedWidget;
}

void UPDFrontendUISubsystem::UnregisterPrimaryLayout()
{
	CreatedPrimaryLayout = nullptr;
}

void UPDFrontendUISubsystem::PushSoftWidgetToStackAsync(const FGameplayTag& WidgetStackTag,
	TSoftClassPtr<UPDActivatableBase> SoftWidgetClass,
	TFunction<void(EAsyncPushWidgetState, UPDActivatableBase*)> AsyncPushStateCallback)
{
	check(!SoftWidgetClass.IsNull());
	
	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		SoftWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([SoftWidgetClass, this, WidgetStackTag, AsyncPushStateCallback]()
		{
			UClass* LoadedSoftWidgetClass = SoftWidgetClass.Get();
			check(LoadedSoftWidgetClass && CreatedPrimaryLayout);
			
			UCommonActivatableWidgetContainerBase* TargetStack = CreatedPrimaryLayout->FindStackByTag(WidgetStackTag);
			UPDActivatableBase* CreatedWidget = TargetStack->AddWidget<UPDActivatableBase>(
					LoadedSoftWidgetClass,
					[AsyncPushStateCallback](UPDActivatableBase& CreatedWidgetInstance)
					{
						AsyncPushStateCallback(EAsyncPushWidgetState::OnCreatedBeforePush,&CreatedWidgetInstance);
					}
				);
			AsyncPushStateCallback(EAsyncPushWidgetState::AfterPush, CreatedWidget);
		}));
}

void UPDFrontendUISubsystem::PopWidgetFromStack(const FGameplayTag& WidgetStackTag)
{
	if (!CreatedPrimaryLayout) return;
	if (UCommonActivatableWidgetContainerBase* FoundStack = CreatedPrimaryLayout->FindStackByTag(WidgetStackTag))
	{
		if (UCommonActivatableWidget* TopWidget = FoundStack->GetActiveWidget())
		{
			TopWidget->DeactivateWidget();
		}
	}
}
