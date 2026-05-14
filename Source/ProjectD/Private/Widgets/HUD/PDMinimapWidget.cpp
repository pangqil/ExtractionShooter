#include "Widgets/HUD/PDMinimapWidget.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Ping/PDPingSubsystem.h"
#include "Ping/PDPingIconWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

void UPDMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UWorld* World = GetWorld())
    {
        if (UPDPingSubsystem* PingSys = World->GetSubsystem<UPDPingSubsystem>())
        {
            PingSys->OnPingAdded.AddDynamic(this, &UPDMinimapWidget::HandlePingAdded);
            PingSys->OnPingRemoved.AddDynamic(this, &UPDMinimapWidget::HandlePingRemoved);

            // 위젯 생성 시점에 이미 떠있는 핑들 그리기
            TArray<FPDPingData> Existing;
            PingSys->GetActivePings(Existing);
            for (const FPDPingData& P : Existing)
            {
                HandlePingAdded(P);
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

    for (auto& Pair : PingIcons)
    {
        if (Pair.Value) Pair.Value->RemoveFromParent();
    }
    PingIcons.Empty();

    Super::NativeDestruct();
}

void UPDMinimapWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
    Super::NativeTick(Geo, DeltaTime);

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    const FVector PlayerLoc = Pawn->GetActorLocation();
    const float PlayerYaw = Pawn->GetActorRotation().Yaw;
    OnPlayerTransformUpdated(PlayerLoc, PlayerYaw);

    // 모든 핑 아이콘 위치 갱신
    if (!PingCanvas) return;
    for (const auto& Pair : PingIcons)
    {
        UPDPingIconWidget* Icon = Pair.Value;
        if (!Icon) continue;

        const FVector2D LocalPos = WorldToMinimap(Icon->WorldLocation);
        if (UCanvasPanelSlot* SlotPtr = Cast<UCanvasPanelSlot>(Icon->Slot))
        {
            SlotPtr->SetPosition(LocalPos);
        }
    }
}

void UPDMinimapWidget::HandlePingAdded(const FPDPingData& PingData)
{
    if (!PingCanvas || !PingIconClass) return;
    if (PingIcons.Contains(PingData.PingId)) return;

    UPDPingIconWidget* Icon = CreateWidget<UPDPingIconWidget>(this, PingIconClass);
    if (!Icon) return;

    Icon->PingId = PingData.PingId;
    Icon->WorldLocation = PingData.WorldLocation;

    if (TObjectPtr<UTexture2D>* TexPtr = IconTextures.Find(PingData.PingType))
    {
        if (UTexture2D* Tex = *TexPtr)
        {
            Icon->SetIconTexture(Tex);
        }
    }

    if (UCanvasPanelSlot* PanelSlot = PingCanvas->AddChildToCanvas(Icon))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        PanelSlot->SetSize(FVector2D(24.f, 24.f));
        PanelSlot->SetPosition(WorldToMinimap(PingData.WorldLocation));
    }

    PingIcons.Add(PingData.PingId, Icon);
}

void UPDMinimapWidget::HandlePingRemoved(int32 PingId)
{
    TObjectPtr<UPDPingIconWidget> Icon;
    if (PingIcons.RemoveAndCopyValue(PingId, Icon))
    {
        if (Icon) Icon->RemoveFromParent();
    }
}

FVector2D UPDMinimapWidget::WorldToMinimap(const FVector& WorldPos) const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PingCanvas) return FVector2D::ZeroVector;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return FVector2D::ZeroVector;

    const FVector PlayerLoc = Pawn->GetActorLocation();
    FVector Delta = WorldPos - PlayerLoc;

    if (bRotateWithPlayer)
    {
        const float Yaw = Pawn->GetActorRotation().Yaw;
        Delta = FRotator(0.f, -Yaw, 0.f).RotateVector(Delta);
    }

    const FVector2D CanvasSize = PingCanvas->GetCachedGeometry().GetLocalSize();
    const float HalfX = CanvasSize.X * 0.5f;
    const float HalfY = CanvasSize.Y * 0.5f;

    // World X(Forward) → Minimap Y- (위)
    // World Y(Right)   → Minimap X+ (오른쪽)
    const float ScreenX = (Delta.Y / MapRadius) * HalfX;
    const float ScreenY = -(Delta.X / MapRadius) * HalfY;

    return FVector2D(ScreenX, ScreenY);
}