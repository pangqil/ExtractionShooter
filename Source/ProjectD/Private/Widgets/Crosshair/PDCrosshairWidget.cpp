// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Crosshair/PDCrosshairWidget.h"
#include "Components/CanvasPanelSlot.h"

void UPDCrosshairWidget::UpdateCrosshair(FVector2D MousePos, float Spread)
{
    // HUD의 Canvas Panel 자식으로 배치돼 있어야 동작. 부모가 Canvas가 아니면 위치 갱신은 무시되고 spread 콜백만 발화.
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetPosition(MousePos);
    }

    if (!FMath::IsNearlyEqual(CurrentSpread, Spread))
    {
        CurrentSpread = Spread;
        OnSpreadChanged(CurrentSpread);
    }
}

void UPDCrosshairWidget::SetCrosshairType(EWeaponType NewType)
{
    if (CurrentType == NewType) return;
    CurrentType = NewType;
    OnCrosshairTypeChanged(CurrentType);
}