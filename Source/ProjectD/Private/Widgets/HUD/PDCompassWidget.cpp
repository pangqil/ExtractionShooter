#include "Widgets/HUD/PDCompassWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPDCompassWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//Image Brush의 Material을 동적 인스턴스로 변환
	if (CompassImage)
	{
		DynamicMat = CompassImage->GetDynamicMaterial();
	}
}

void UPDCompassWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
	Super::NativeTick(Geo, DeltaTime);

	if (!DynamicMat || !CompassImage) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	//화면 중앙 기준 마우스 위치 => Yaw 계산 
	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY)) return;

	int32 ViewportSizeX = 0, ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0) return;

	const FVector2D Center(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
	const FVector2D Delta(MouseX - Center.X, MouseY - Center.Y);

	//화면 위쪽(-Y) = 0도(N), 오른쪽(+X) = 90도(E), 아래(+Y) = 180도(S), 왼쪽(-X) = 270도(W)
	float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.X, -Delta.Y));
	while (Yaw < 0.f) Yaw += 360.f;
	while (Yaw >= 360.f) Yaw -= 360.f;

	//표시 영역 너비 vs 텍스처 전체 너비 비율
	const FVector2D ShowSize = CompassImage->GetCachedGeometry().GetLocalSize();
	const float Scale = (TextureFullWidth > 0.f) ? (ShowSize.X / TextureFullWidth) : 1.f;

	//UV 스크롤 값: 표시 영역 중앙이 텍스처의 Yaw 위치를 가리키게
	const float NormalizedYaw = Yaw / 360.f;
	const float UVOffset = NormalizedYaw - Scale * 0.5f;

	DynamicMat->SetScalarParameterValue(TEXT("UVOffset"), UVOffset);
	DynamicMat->SetScalarParameterValue(TEXT("UVScale"), Scale);
}