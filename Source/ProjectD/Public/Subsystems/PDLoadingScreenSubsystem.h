// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "PDLoadingScreenSubsystem.generated.h"

class UPDLoadingScreenWidget;
class UTexture2D;
struct FWorldContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnLoadingReasonUpdated, const FText&, NewReason);

UCLASS()
class PROJECTD_API UPDLoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintCallable, Category = "PD|LoadingScreen")
	void SetLoadingReason(const FText& InReason);

	/** 다음 로딩 1회에만 적용되는 splash override. 풀/Static을 무시. 사용 후 자동 클리어. */
	void SetNextSplashOverride(TSoftObjectPtr<UTexture2D> InTexture);

	UPROPERTY(BlueprintAssignable, Category = "PD|LoadingScreen")
	FPDOnLoadingReasonUpdated OnLoadingReasonUpdated;

private:
	void HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName);
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);

	void ShowLoadingScreen();
	void HideLoadingScreen();
	bool ShouldDisplayInCurrentEnvironment() const;

	/** 우선순위: PendingSplashOverride > Settings.StaticSplashOverride > Pool 랜덤 > null. Pending이면 내부에서 클리어. */
	TSoftObjectPtr<UTexture2D> ResolveSplashTexture();

	UPROPERTY(Transient)
	TObjectPtr<UPDLoadingScreenWidget> ActiveWidget;

	TSoftObjectPtr<UTexture2D> PendingSplashOverride;

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;

	float HoldElapsed = 0.f;
	bool bHolding = false;

	FText CurrentReason;
};