// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Crosshair/PDCrosshairWidget.h"

void UPDCrosshairWidget::UpdateCrosshair(FVector2D MousePos, float Spread)
{
    // 위젯 위치를 마우스 위치로 이동
    SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
    SetPositionInViewport(MousePos, true);

    CurrentSpread = Spread;
    OnSpreadChanged(CurrentSpread);
   
}

void UPDCrosshairWidget::SetCrosshairType(EWeaponType NewType)
{
    if (CurrentType == NewType) return;
    CurrentType = NewType;
    OnCrosshairTypeChanged(CurrentType);
}