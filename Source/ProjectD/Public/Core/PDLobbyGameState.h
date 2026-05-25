// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PDLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnLobbyPlayersChanged);

/**
 * Lobby/StartupLevel용 GameState.
 * AGameStateBase::PlayerArray가 이미 replicate되므로 별도 닉네임 리스트를 두지 않는다.
 * Add/RemovePlayerState 시 LobbyVersion을 증가시켜 클라이언트의 OnRep에서
 * OnLobbyPlayersChanged 델리게이트가 브로드캐스트되도록 한다.
 */
UCLASS()
class PROJECTD_API APDLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "PD|Lobby")
	FPDOnLobbyPlayersChanged OnLobbyPlayersChanged;

	/** 방을 연 호스트의 PlayerState. 첫 AddPlayerState로 결정. 클라이언트에도 replicate됨. */
	UFUNCTION(BlueprintPure, Category = "PD|Lobby")
	APlayerState* GetHostPlayerState() const { return HostPlayerState; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyVersion)
	int32 LobbyVersion = 0;

	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> HostPlayerState;

	UFUNCTION()
	void OnRep_LobbyVersion();
};