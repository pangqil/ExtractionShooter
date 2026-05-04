// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PDActivatableBase.generated.h"

/**
 * 모든 위젯의 부모 클래스.
 */

// 각 위젯이 활성화될 때 게임 입력을 어떻게 처리할지 정의하는 열거형
UENUM(BlueprintType)
enum class EWidgetInputMode : uint8
{
	Game     UMETA(DisplayName = "Game Only"),  // 게임 입력 그대로
	GameAndMenu UMETA(DisplayName = "Game And Menu"),   // HUD 위에 떠도 게임 계속
	Menu        UMETA(DisplayName = "Menu Only")        // 인벤토리/맵 — 게임 입력 차단, 마우스 노출
};

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDActivatableBase : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	EWidgetInputMode InputMode{EWidgetInputMode::Menu};
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bShowMouseCursor{true};
};
