// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "PDPrimaryLayout.generated.h"

class UCommonActivatableWidgetContainerBase;
/**
 *  Viewport에 해당 위젯만 추가하고, 내부에서 스택을 관리하는 레이아웃 위젯입니다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	/** Subsystem에서 Push 대상 스택을 찾을 때 사용. */
	UCommonActivatableWidgetContainerBase* FindStackByTag(const FGameplayTag& StackTag) const;
	
protected:
	/** BP에서 BindWidget으로 묶은 스택을 태그와 함께 등록. */
	UFUNCTION(BlueprintCallable, Category = "Layout")
	void RegisterStack(UPARAM(meta = (Categories = "Frontend.WidgetStack"))FGameplayTag StackTag, UCommonActivatableWidgetContainerBase* Stack);

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> WidgetStackMap;
};
