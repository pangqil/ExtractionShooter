// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PDGameInstance.generated.h"

class UPDPrimaryLayout;

USTRUCT(BlueprintType)
struct FPDPlayerData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold=0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Experience=0;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// TArray<FName, int32> StashedItemsIDs;
	
	//Skill Data, Activable Weapon? or....
};

UCLASS()
class PROJECTD_API UPDGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "PD|Data")
	void SavePlayerData(const FPDPlayerData& InData);

	UFUNCTION(BlueprintCallable, Category = "PD|Data")
	FPDPlayerData LoadPlayerData() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PD|UI")
	TSoftClassPtr<UPDPrimaryLayout> PrimaryLayoutClass;

	UPROPERTY()
	FPDPlayerData PlayerData;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPDPrimaryLayout> PrimaryLayoutWidget;
};
