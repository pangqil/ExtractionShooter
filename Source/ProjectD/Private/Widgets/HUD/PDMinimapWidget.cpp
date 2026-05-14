#include "Widgets/HUD/PDMinimapWidget.h"
#include "GameFramework/PlayerController.h"
#include "Ping/PDPingSubsystem.h"
#include "GameFramework/Pawn.h"

void UPDMinimapWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
	Super::NativeTick(Geo, DeltaTime);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			const FVector Loc = Pawn->GetActorLocation();
			const float Yaw = Pawn->GetActorRotation().Yaw;
			OnPlayerTransformUpdated(Loc, Yaw);
		}
	}
}

void UPDMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		if (UPDPingSubsystem* PingSys = World->GetSubsystem<UPDPingSubsystem>())
		{
			//Subsystem의 델리게이트 구독
			PingSys->OnPingAdded.AddDynamic(this, &UPDMinimapWidget::HandlePingAdded);
			PingSys->OnPingRemoved.AddDynamic(this, &UPDMinimapWidget::HandlePingRemoved);

			//위젯 생성 시점에 이미 떠있는 핑들 그리기
			TArray<FPDPingData> Existing;
			PingSys->GetActivePings(Existing);
			for (const FPDPingData& P : Existing)
			{
				OnPingAddedToMinimap(P);
			}
		}
	}
}

void UPDMinimapWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (UPDPingSubsystem* PingSys = World->GetSubsystem<UPDPingSubsystem>())
		{
			PingSys->OnPingAdded.RemoveDynamic(this, &UPDMinimapWidget::HandlePingAdded);
			PingSys->OnPingRemoved.RemoveDynamic(this, &UPDMinimapWidget::HandlePingRemoved);
		}
	}
	Super::NativeDestruct();
}

void UPDMinimapWidget::HandlePingAdded(const FPDPingData& PingData)
{
	OnPingAddedToMinimap(PingData);
}

void UPDMinimapWidget::HandlePingRemoved(int32 PingId)
{
	OnPingRemovedFromMinimap(PingId);
}