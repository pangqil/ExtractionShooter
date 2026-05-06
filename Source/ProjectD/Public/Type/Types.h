#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Types.generated.h"

USTRUCT(BlueprintType)
struct FPDPlayerData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Experience = 0;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// TArray<FName, int32> StashedItemsIDs;
	
	// Skill Data, Activable Weapon? or....
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
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;
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

// 참고: EPDEnemyState 은 Enemy 시스템 전용 enum 으로 분리되어
//      Public/Enemy/Types/EnemyTypes.h 로 이동. 여기에는 더 이상 정의하지 않음.

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

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None,
	Rifle,
	Shotgun,
	Sniper
};

/** 플레이어 무기 슬롯 (0 = None, 슬롯 인덱스와 1:1 매핑) */
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	None,
	Slot1_Rifle,
	Slot2_Shotgun,
	Slot3_Sniper
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single,
	Auto
};

/** 무기 레벨별 스탯. 자식 생성자에서 LevelStats.Add({...}) 형태로 채운다. */
USTRUCT(BlueprintType)
struct FWeaponLevelStats
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Damage = 0.f;

	/** 연사 간격(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FireRate = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Range = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxAmmo = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ReloadTime = 2.f;

	/** 0~1. 1=완벽. 낮을수록 탄 퍼짐 커짐 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Accuracy = 0.9f;
};
