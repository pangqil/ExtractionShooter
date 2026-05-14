#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDWorldMapWidget.generated.h"

class UCanvasPanel;
class UImage;
class UPDWorldMapDataAsset;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDWorldMapWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	//레벨별 맵 정보 데이터에셋. BP에서 DA_WorldMap 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	TObjectPtr<UPDWorldMapDataAsset> WorldMapData;

	//맵이 캡처된 영역의 월드 중심 (X, Y). DataAsset 없을 때 fallback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	FVector2D MapWorldCenter = FVector2D::ZeroVector;

	//캡처 카메라의 OrthoWidth (cm). DataAsset 없을 때 fallback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap", meta=(ClampMin="100.0"))
	float MapWorldSize = 10000.f;

	//플레이어 화살표 회전 보정값. DataAsset 없을 때 fallback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	float PlayerArrowAngleOffset = 0.f;

protected:
	//Canvas Panel 
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> MapCanvas;

	//Image 맵 배경 
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> MapBackground;

	//Image 플레이어 화살표
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> PlayerArrow;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

private:
	//월드 좌표 => 맵 캔버스 로컬 좌표 변환
	FVector2D WorldToMap(const FVector& WorldPos) const;
};