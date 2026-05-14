#include "Widgets/HUD/PDWorldMapWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

void UPDWorldMapWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
	Super::NativeTick(Geo, DeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn || !PlayerArrow) return;

	const FVector PlayerLoc = Pawn->GetActorLocation();
	const float PlayerYaw = Pawn->GetActorRotation().Yaw;

	//플레이어 화살표 위치 갱신
	if (UCanvasPanelSlot* PlayerSlot = Cast<UCanvasPanelSlot>(PlayerArrow->Slot))
	{
		PlayerSlot->SetPosition(WorldToMap(PlayerLoc));
	}

	//화살표 회전 (플레이어 방향 가리키기)
	PlayerArrow->SetRenderTransformAngle(PlayerYaw);
}

FVector2D UPDWorldMapWidget::WorldToMap(const FVector& WorldPos) const
{
	if (!MapCanvas || MapWorldSize <= 0.f) return FVector2D::ZeroVector;

	//맵 중심 기준 상대 좌표
	const FVector2D Delta(WorldPos.X - MapWorldCenter.X, WorldPos.Y - MapWorldCenter.Y);

	const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
	const float HalfX = CanvasSize.X * 0.5f;
	const float HalfY = CanvasSize.Y * 0.5f;

	//World X(Forward/North)
	//World Y(Right/East)    
	const float HalfWorld = MapWorldSize * 0.5f;
	const float ScreenX = (Delta.Y / HalfWorld) * HalfX;
	const float ScreenY = -(Delta.X / HalfWorld) * HalfY;

	return FVector2D(ScreenX, ScreenY);
}