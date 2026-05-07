// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "AttributeSet.h"
#include "PDHUDWidget.generated.h"

class UPDAttributeBarWidget;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * In-Game HUD. Frontend.WidgetStack.GameHud 스택에 push되어 viewport 최하단에 깔리는 활성 위젯.
 * Pawn의 ASC에서 attribute 변경 델리게이트를 구독해 자식 Bar 위젯들을 push 방식으로 갱신한다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDHUDWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDAttributeBarWidget> Bar_Health;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Stamina;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Hunger;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Thirst;

private:
	struct FBoundHandle
	{
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};

	void TryBindToOwningPawnASC();

	void BindAttributeToBar(UPDAttributeBarWidget* Bar,
		const FGameplayAttribute& CurrentAttr,
		const FGameplayAttribute& MaxAttr);

	void RefreshBar(UPDAttributeBarWidget* Bar,
		const FGameplayAttribute& CurrentAttr,
		const FGameplayAttribute& MaxAttr) const;

	void UnbindAll();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TArray<FBoundHandle> BoundHandles;
};
