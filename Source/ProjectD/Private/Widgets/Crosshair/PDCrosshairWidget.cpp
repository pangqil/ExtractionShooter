// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Crosshair/PDCrosshairWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UPDCrosshairWidget::UpdateCrosshair(FVector2D MousePos, float Spread)
{
    // HUD의 Canvas Panel 자식으로 배치돼 있어야 동작. 부모가 Canvas가 아니면 위치 갱신은 무시되고 spread 콜백만 발화.
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        // GetMousePosition은 viewport 픽셀 좌표를 주지만 CanvasSlot 좌표는 DPI 스케일이 적용된 Slate 단위.
        // 해상도/DPI가 다른 환경에서 크로스헤어가 어긋나는 것을 막기 위해 viewport scale로 보정한다.
        const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
        const FVector2D LocalPos = (DPIScale > KINDA_SMALL_NUMBER) ? (MousePos / DPIScale) : MousePos;

        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetPosition(LocalPos);
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