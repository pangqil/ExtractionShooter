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
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	//캐릭터 Yaw => 0~360 범위로 정규화
	float Yaw = Pawn->GetActorRotation().Yaw;
	while (Yaw < 0.f) Yaw += 360.f;
	while (Yaw >= 360.f) Yaw -= 360.f;

	//표시 영역 너비 vs 텍스처 전체 너비 비율
	const FVector2D ShowSize = CompassImage->GetCachedGeometry().GetLocalSize();
	const float Scale = (TextureFullWidth > 0.f) ? (ShowSize.X / TextureFullWidth) : 1.f;

	//UV 스크롤 값: 표시 영역 중앙(UV 0.5)이 텍스처의 Yaw 위치를 가리키게
	const float NormalizedYaw = Yaw / 360.f;
	const float UVOffset = NormalizedYaw - Scale * 0.5f;

	DynamicMat->SetScalarParameterValue(TEXT("UVOffset"), UVOffset);
	DynamicMat->SetScalarParameterValue(TEXT("UVScale"), Scale);
}