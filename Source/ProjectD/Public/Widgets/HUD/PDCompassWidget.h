#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDCompassWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDCompassWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	//텍스처 한 바퀴
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compass")
	float TextureFullWidth = 1440.f;

protected:
	//Image (Material 적용된 거)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> CompassImage;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

private:
	//런타임 Material 인스턴스 (UV 파라미터 동적 조작용)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat;
};