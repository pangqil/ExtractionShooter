#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Ping/PDPingTypes.h"
#include "PDMinimapWidget.generated.h"

class UCanvasPanel;
class UPDPingIconWidget;
class UTexture2D;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDMinimapWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
	float MapRadius = 2000.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
	TSubclassOf<UPDPingIconWidget> PingIconClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
	TMap<EPDPingType, TObjectPtr<UTexture2D>> IconTextures;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
	bool bRotateWithPlayer = false;

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> PingCanvas;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

	//BP가 플레이어 정보로 화살표 갱신 등 처리
	UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
	void OnPlayerTransformUpdated(const FVector& Location, float Yaw);

	UFUNCTION()
	void HandlePingAdded(const FPDPingData& PingData);

	UFUNCTION()
	void HandlePingRemoved(int32 PingId);

private:
	/** 활성 핑 위젯 추적. PingId → 위젯. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UPDPingIconWidget>> PingIcons;

	/** 월드 좌표 → 미니맵 캔버스 로컬 좌표 변환. */
	FVector2D WorldToMinimap(const FVector& WorldPos) const;
};