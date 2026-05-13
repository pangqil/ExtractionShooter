// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDBodyPartImageWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPDBodyPartImageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Part)
	{
		if (PartMaterial)
		{
			// 인스턴스마다 다른 MI 적용 — Brush 디폴트보다 우선해야 GetDynamicMaterial이 이 MI를 기반으로 MID를 만든다.
			Image_Part->SetBrushFromMaterial(PartMaterial);
		}
		// GetDynamicMaterial: 이미 MID면 그대로 반환, 아니면 Brush 머터리얼로부터 생성해 박아준다.
		PartMID = Image_Part->GetDynamicMaterial();
	}
}

void UPDBodyPartImageWidget::OnValuesUpdated(float Current, float Max, float Percent)
{
	if (PartMID)
	{
		PartMID->SetScalarParameterValue(PercentParamName, Percent);
	}
}