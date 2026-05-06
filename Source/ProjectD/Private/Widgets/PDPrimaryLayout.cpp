// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PDPrimaryLayout.h"

UCommonActivatableWidgetContainerBase* UPDPrimaryLayout::FindStackByTag(const FGameplayTag& StackTag) const
{
	checkf(WidgetStackMap.Contains(StackTag), TEXT("태그에 맞는 스택이 존재하지 않습니다."));
	
	return WidgetStackMap.FindRef(StackTag);
}

void UPDPrimaryLayout::RegisterStack(UPARAM(meta = (Categories = "Frontend.WidgetStack"))FGameplayTag StackTag, UCommonActivatableWidgetContainerBase* Stack)
{
	if (!IsDesignTime())
	{
		if (!WidgetStackMap.Contains(StackTag))
		{
			WidgetStackMap.Add(StackTag, Stack);
		}
	}
}
