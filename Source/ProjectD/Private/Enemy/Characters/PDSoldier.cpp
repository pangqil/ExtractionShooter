#include "Enemy/Characters/PDSoldier.h"

#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Items/PDStashActor.h"
#include "Items/PDStashComponent.h"
#include "TimerManager.h"
#include "Weapons/PDWeaponBase.h"

APDSoldier::APDSoldier()
{
	TeamID = 2; // Hostile.
}

void APDSoldier::BeginPlay()
{
	Super::BeginPlay();

	SpawnAndEquipDefaultWeapon();

	// 타겟 획득/상실에 맞춰 풀오토 루프 on/off.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnTargetChanged.AddDynamic(this, &APDSoldier::HandleTargetChanged);
	}
}

void APDSoldier::OnEnterState_Dead()
{
	StopContinuousFire();

	Super::OnEnterState_Dead();

	if (!EquippedWeapon) return;

	EquippedWeapon->OnUnequip();

	// 베이스가 스폰한 시체 컨테이너가 Stash 류면 무기 데이터를 그 안으로 이전. 픽업은 LootBox 상호작용으로만.
	bool bTransferred = false;
	if (APDStashActor* Corpse = Cast<APDStashActor>(GetCorpseContainer()))
	{
		if (UPDStashComponent* Stash = Corpse->GetStashComponent())
		{
			const FPDItemData& WeaponData = EquippedWeapon->GetCachedItemData();
			if (!WeaponData.ItemID.IsNone())
			{
				Stash->AddItemPartial(WeaponData, 1);
				bTransferred = true;
			}
		}
	}

	if (!bTransferred)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] CorpseContainer 가 Stash 류가 아니거나 WeaponData 미지정 — 장착무기 소실."),
			*GetName());
	}

	EquippedWeapon->Destroy();
	EquippedWeapon = nullptr;
}

void APDSoldier::SpawnAndEquipDefaultWeapon()
{
	if (!DefaultWeaponClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDWeaponBase* NewWeapon = World->SpawnActor<APDWeaponBase>(
		DefaultWeaponClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParams);

	if (!NewWeapon) return;

	SetEquippedWeapon(NewWeapon, /*bDestroyPrevious=*/true);
}

void APDSoldier::SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious)
{
	if (EquippedWeapon == NewWeapon) return;

	if (EquippedWeapon)
	{
		EquippedWeapon->OnUnequip();
		if (bDestroyPrevious)
		{
			EquippedWeapon->Destroy();
		}
	}

	EquippedWeapon = NewWeapon;

	if (EquippedWeapon)
	{
		AttachActorToWeaponSocket(EquippedWeapon);
		EquippedWeapon->OnEquip(this);
	}
}

void APDSoldier::HandleTargetChanged(AActor* NewTarget)
{
	if (!bAutoFireOnAttackRequested) return;

	if (NewTarget && EquippedWeapon)
	{
		StartContinuousFire();
	}
	else
	{
		StopContinuousFire();
	}
}

void APDSoldier::StartContinuousFire()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (World->GetTimerManager().IsTimerActive(FireTimerHandle)) return;

	// 첫 발은 지연 없이 즉시 시도.
	OnFireTick();

	// 0 입력 방지용 최소 1프레임 클램프.
	const float Interval = FMath::Max(FireInterval, 0.0167f);
	World->GetTimerManager().SetTimer(FireTimerHandle, this, &APDSoldier::OnFireTick, Interval, /*bLoop=*/true);
}

void APDSoldier::StopContinuousFire()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}
}

void APDSoldier::OnFireTick()
{
	if (!EquippedWeapon)
	{
		StopContinuousFire();
		return;
	}

	UPDCombatComponent* Combat = GetCombatComponent();
	if (!Combat || !Combat->HasValidTarget())
	{
		StopContinuousFire();
		return;
	}

	if (bRequireInRangeToFire)
	{
		if (const AActor* Target = Combat->GetCurrentTarget())
		{
			const float Range = Combat->GetAttackRange();
			const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
			if (DistSq > Range * Range) return; // 사거리 밖이면 이번 틱 스킵, 루프는 유지.
		}
	}

	EquippedWeapon->Fire();
}
