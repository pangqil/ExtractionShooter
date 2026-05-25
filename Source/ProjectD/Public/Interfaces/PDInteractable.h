#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDInteractable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	FText GetInteractDisplayText() const;

	// override 안 한 클래스는 빈 텍스트 — 위젯엔 키 아이콘만 표시되고 액션 라벨 자리는 공백.
	virtual FText GetInteractDisplayText_Implementation() const { return FText::GetEmpty(); }

	// 액터 메시별 위치 미세조정용. BP에서 override해 액터 머리/문/창고 윗부분 등으로 세팅.

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	FVector GetInteractPromptOffset() const;

	// override 안 한 클래스는 액터 origin 사용 (오프셋 0).
	virtual FVector GetInteractPromptOffset_Implementation() const { return FVector::ZeroVector; }
};
