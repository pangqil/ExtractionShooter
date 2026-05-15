#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Templates/SubclassOf.h"
#include "Types.generated.h"

class APDWeaponBase;

UENUM(BlueprintType)
enum class EPDItemType : uint8
{
	Equipment  UMETA(DisplayName = "Equipment"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc       UMETA(DisplayName = "Misc"),
};

UENUM(BlueprintType)
enum class EPDItemFilterTab : uint8
{
	Equipment  UMETA(DisplayName = "Equipment"),
	Consumable UMETA(DisplayName = "Consumable"),
	Misc       UMETA(DisplayName = "Misc"),
};



UENUM(BlueprintType)
enum class EPDItemSortMode : uint8
{
	None UMETA(DisplayName = "None"),
	Name UMETA(DisplayName = "Name"),
	Type UMETA(DisplayName = "Type"),
};

UENUM(BlueprintType)
enum class EPDEquipmentSlotType : uint8
{
	None   UMETA(DisplayName = "None"),
	Weapon UMETA(DisplayName = "Weapon"),
	Head   UMETA(DisplayName = "Head"),
	Armor  UMETA(DisplayName = "Armor"),
	Bag    UMETA(DisplayName = "Bag"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None    UMETA(DisplayName = "None"),
	Rifle   UMETA(DisplayName = "Rifle"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Sniper  UMETA(DisplayName = "Sniper"),
	Pistol  UMETA(DisplayName = "Pistol"),
};

USTRUCT(BlueprintType)
struct FPDItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDItemType ItemType = EPDItemType::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsQuestItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BuyPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SellPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	// 장비 아이템일 때 어느 장비 슬롯에 들어갈지 지정. WeaponType은 전투 타입 구분용으로만 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDEquipmentSlotType EquipmentSlotType = EPDEquipmentSlotType::None;

	// 무기 아이템일 때만 설정. nullptr이면 비-무기 아이템(소비/잡템).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<APDWeaponBase> WeaponClass;

	// 무기 아이템일 때 전투 타입을 지정. 장착 위치는 EquipmentSlotType으로 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponType WeaponType = EWeaponType::None;
};

USTRUCT(BlueprintType)
struct FPDInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEmpty = true;

	bool IsEmpty() const
	{
		return bIsEmpty || Quantity <= 0 || ItemData.ItemID.IsNone();
	}

	void Clear()
	{
		ItemData = FPDItemData();
		Quantity = 0;
		bIsEmpty = true;
	}
};


USTRUCT(BlueprintType)
struct FPDPlayerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Experience = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDInventorySlot> StashItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StashUpgradeLevel = 0;
};

USTRUCT(BlueprintType)
struct FPDEquippedItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDEquipmentSlotType SlotType = EPDEquipmentSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDInventorySlot ItemSlot;

	bool IsEmpty() const
	{
		return SlotType == EPDEquipmentSlotType::None || ItemSlot.IsEmpty();
	}

	void Clear()
	{
		SlotType = EPDEquipmentSlotType::None;
		ItemSlot.Clear();
	}
};

USTRUCT(BlueprintType)
struct FPDDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDamageType> DamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;
};

// EPDEnemyState 은 Enemy 시스템 전용 enum 으로 분리되어
// Public/Enemy/Types/EnemyTypes.h 로 이동.

UENUM(BlueprintType)
enum class EBodyPart : uint8
{
	None,
	Head,
	Torso,
	Arm_L,
	Arm_R,
	Leg_L,
	Leg_R
};

USTRUCT(BlueprintType)
struct FBodyPartMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName HitBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBodyPart BodyPart = EBodyPart::None;
};

UENUM(BlueprintType)
enum class ERaidState : uint8
{
	Idle,
	InProgress,
	Extracting,
	Ended
};

// ─── 무기 시스템 ────────────────────────────────────────────────
// EWeaponType은 FPDItemData보다 위에 정의됨(FPDItemData::WeaponType 기본값에서 사용).

// 슬롯 인덱스: Slot1_Rifle=0, Slot2_Shotgun=1, Slot3_Sniper=2, Slot4_Pistol=3 (배열 인덱스로 직접 사용)
// None=4 은 "선택 없음" 센티넬. 인덱스로 쓰기 전에 None 체크 필수.
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Slot1_Rifle   UMETA(DisplayName = "Rifle"),
	Slot2_Shotgun UMETA(DisplayName = "Shotgun"),
	Slot3_Sniper  UMETA(DisplayName = "Sniper"),
	Slot4_Pistol  UMETA(DisplayName = "Pistol"),
	None          UMETA(DisplayName = "None"),
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto   UMETA(DisplayName = "Auto"),
	Single UMETA(DisplayName = "Single"),
};

USTRUCT(BlueprintType)
struct FWeaponLevelStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float FireRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Range = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxAmmo = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ReloadTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Accuracy = 0.95f;
};


UENUM(BlueprintType)
enum class EUILayer : uint8
{
	Frontend UMETA(DisplayName = "Frontend"), 
	GameMenu UMETA(DisplayName = "Game Menu"),
	Modal    UMETA(DisplayName = "Modal"),
};
