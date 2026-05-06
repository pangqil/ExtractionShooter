// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PDAttributeBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * GAS Attribute의 Current/Max 값을 받아 ProgressBar로만 표시하는 재사용 컴포넌트.
 * ASC를 직접 참조하지 않음 — 상위 HUD가 push해 준다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDAttributeBarWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|HUD")
	void SetValues(float Current, float Max);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Value;
};
