#include "Enemy/Characters/PDSoldier.h"

#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Weapons/PDWeaponBase.h"

APDSoldier::APDSoldier()
{
	TeamID = 2; // Hostile.
}

void APDSoldier::BeginPlay()
{
	Super::BeginPlay();

	SpawnAndEquipDefaultWeapon();

	// CombatComponent.OnAttackRequested 는 "공격 의도" 신호 — 무기에게 Fire 위임.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &APDSoldier::HandleAttackRequested);
	}
}

void APDSoldier::OnEnterState_Dead()
{
	// Super 안의 SpawnCorpseContainer → HarvestEquippedWeaponSlots 가 EquippedWeapon 살아있을 때 슬롯을 추출한다.
	Super::OnEnterState_Dead();

	// 데이터 추출이 끝났으므로 무기 액터는 정리. Enemy 본체도 곧 SetLifeSpan으로 소멸되므로 부착 상태로 남기지 않는다.
	if (EquippedWeapon)
	{
		EquippedWeapon->OnUnequip();
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}
}

void APDSoldier::HarvestEquippedWeaponSlots(TArray<FPDInventorySlot>& OutSlots) const
{
	if (!EquippedWeapon) return;

	const FPDItemData& Data = EquippedWeapon->GetCachedItemData();
	if (Data.ItemID.IsNone()) return;

	FPDInventorySlot Slot;
	Slot.ItemData = Data;
	Slot.Quantity = 1;
	Slot.bIsEmpty = false;
	OutSlots.Add(Slot);
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

void APDSoldier::HandleAttackRequested(AActor* /*Target*/)
{
	if (!bAutoFireOnAttackRequested) return;
	if (!EquippedWeapon) return;

	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}

	EquippedWeapon->Fire();
}
