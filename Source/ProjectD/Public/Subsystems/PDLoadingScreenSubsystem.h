// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "PDLoadingScreenSubsystem.generated.h"

class UPDLoadingScreenWidget;
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

	UPROPERTY(BlueprintAssignable, Category = "PD|LoadingScreen")
	FPDOnLoadingReasonUpdated OnLoadingReasonUpdated;

private:
	void HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName);
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);

	void ShowLoadingScreen();
	void HideLoadingScreen();
	bool ShouldDisplayInCurrentEnvironment() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDLoadingScreenWidget> ActiveWidget;

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;

	float HoldElapsed = 0.f;
	bool bHolding = false;

	FText CurrentReason;
};