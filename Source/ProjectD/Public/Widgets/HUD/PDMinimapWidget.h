#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "Ping/PDPingTypes.h"
#include "PDMinimapWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDMinimapWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	//미니맵 표시 반경 (월드 단위, cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
	float MapRadius = 2000.f;

protected:
	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	//BP에서 미니맵에 핑 아이콘 그리기
	UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
	void OnPingAddedToMinimap(const FPDPingData& PingData);

	//BP에서 핑 아이콘 제거
	UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
	void OnPingRemovedFromMinimap(int32 PingId);

	//BP에서 갱신 (플레이어 위치/회전)
	UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
	void OnPlayerTransformUpdated(const FVector& Location, float Yaw);
	
	UFUNCTION()
	void HandlePingAdded(const FPDPingData& PingData);

	UFUNCTION()
	void HandlePingRemoved(int32 PingId);
};