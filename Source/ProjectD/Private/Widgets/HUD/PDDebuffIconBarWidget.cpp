// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDDebuffIconBarWidget.h"

#include "Components/PanelWidget.h"
#include "Widgets/HUD/PDDebuffIconWidget.h"

void UPDDebuffIconBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// WBP 디자인 프리뷰용 placeholder 자식이 남아있을 수 있으니 비움.
	if (Container_Icons)
	{
		Container_Icons->ClearChildren();
	}
	ActiveIcons.Reset();
}

void UPDDebuffIconBarWidget::SetDebuffActive(const FGameplayTag& DebuffTag, bool bActive)
{
	if (!Container_Icons) return;

	const FPDDebuffIconData* Data = TagToIconData.Find(DebuffTag);
	if (!Data) return;

	const bool bCurrentlyActive = ActiveIcons.Contains(DebuffTag);

	if (bActive && !bCurrentlyActive)
	{
		if (!IconWidgetClass) return;

		UPDDebuffIconWidget* NewIcon = CreateWidget<UPDDebuffIconWidget>(this, IconWidgetClass);
		if (!NewIcon) return;

		NewIcon->SetIconData(Data->IconMaterial);

		// AddChild = 박스 끝에 추가. VerticalBox면 맨 아래, HorizontalBox면 맨 오른쪽.
		// 결과: 먼저 걸린 디버프가 상단/앞 유지, 새 디버프가 그 뒤로 누적.
		Container_Icons->AddChild(NewIcon);
		ActiveIcons.Add(DebuffTag, NewIcon);
	}
	else if (!bActive && bCurrentlyActive)
	{
		TObjectPtr<UPDDebuffIconWidget> Existing = ActiveIcons.FindRef(DebuffTag);
		if (Existing)
		{
			// RemoveChild 후 UMG Box가 남은 자식들을 자동 reflow → 빈 자리 채워짐.
			Container_Icons->RemoveChild(Existing);
		}
		ActiveIcons.Remove(DebuffTag);
	}
}
