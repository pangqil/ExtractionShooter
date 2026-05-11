// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PDWeaponComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/EngineTypes.h"

UPDWeaponComponent::UPDWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPDWeaponComponent::SetAimTarget(AActor* Target)
{
    AimTarget = Target;
}

void UPDWeaponComponent::ClearAimTarget()
{
    AimTarget = nullptr;
}

FVector UPDWeaponComponent::GetAimDirection(const FVector& StartLocation) const
{
    AActor* Owner = GetOwner();
    if (!Owner) return FVector::ForwardVector;

    // ── 적: AimTarget이 설정된 경우 ────────────────────────────────────
    if (AimTarget.IsValid())
    {
        FVector TargetLoc = AimTarget->GetActorLocation() + FVector(0.f, 0.f, 80.f);
        FVector Dir = TargetLoc - StartLocation;
        if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
    }

    // ── 플레이어: PlayerController 커서 ────────────────────────────────
    APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
    if (PC)
    {
        // 1순위: 커서가 Pawn 위 → 부위 직접 조준
        FHitResult PawnHit;
        if (PC->GetHitResultUnderCursorForObjects(
            { UEngineTypes::ConvertToObjectType(ECC_Pawn) }, true, PawnHit)
            && PawnHit.GetActor() && PawnHit.GetActor() != Owner)
        {
            FVector Dir = PawnHit.Location - StartLocation;
            if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
        }
        // 2순위: 지면 커서
        FHitResult CursorHit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
        {
            FVector Dir = CursorHit.Location - StartLocation;
            if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
        }
    }

    // ── 폴백: Owner 전방 벡터 ───────────────────────────────────────────
    return Owner->GetActorForwardVector();
}


