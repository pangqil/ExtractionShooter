#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDWorldMapWidget.generated.h"

class UCanvasPanel;
class UImage;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDWorldMapWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	//맵이 캡처된 영역의 월드 중심 (X, Y) BP에서 레벨 맞춰 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	FVector2D MapWorldCenter = FVector2D::ZeroVector;

	//맵 캡처 카메라의 OrthoWidth (월드 영역 너비, cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap", meta=(ClampMin="100.0"))
	float MapWorldSize = 10000.f;

protected:
	//Canvas Panel => 맵 영역
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> MapCanvas;

	//Image => 플레이어 위치
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> PlayerArrow;

	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

private:
	//월드 좌표 => 맵 캔버스 로컬 좌표 변환
	FVector2D WorldToMap(const FVector& WorldPos) const;
};