// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PDFrontendUISubsystem.generated.h"

struct FGameplayTag;
class UPDActivatableBase;
class UPDPrimaryLayout;

UENUM(BlueprintType)
enum class EAsyncPushWidgetState : uint8
{
	OnCreatedBeforePush,
	AfterPush
};
/**
 * 위젯을 환경에 구애받지 않고 스택에 할당할 수 있도록 하는 서브 시스템
 */
UCLASS()
class PROJECTD_API UPDFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	static UPDFrontendUISubsystem* Get(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RegisterCreatePrimaryLayoutWidget(UPDPrimaryLayout* CreatedWidget);
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UnregisterPrimaryLayout();
	
	void PushSoftWidgetToStackAsync(const FGameplayTag& WidgetStackTag,TSoftClassPtr<UPDActivatableBase> SoftWidgetClass,TFunction<void(EAsyncPushWidgetState,UPDActivatableBase*)> AsyncPushStateCallback);
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void PopWidgetFromStack(const FGameplayTag& WidgetStackTag);
private:
	UPROPERTY(Transient)
	TObjectPtr<UPDPrimaryLayout> CreatedPrimaryLayout;
};
