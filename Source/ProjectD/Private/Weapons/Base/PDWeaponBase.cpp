#include "Weapons/Base/PDWeaponBase.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Characters/PDPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTag/PDGameplayTags.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Component/PDWeaponComponent.h"

#include "Items/PDInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Texture2D.h"

APDWeaponBase::APDWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	SetReplicateMovement(true);
	bAlwaysRelevant = true;
	NetDormancy = DORM_Never;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->SetupAttachment(RootComponent);
	PickupCollision->SetSphereRadius(80.f);

	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickupCollision->SetGenerateOverlapEvents(false);
}

void APDWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	SetReplicates(true);
	SetReplicateMovement(true);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_Never);

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void APDWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDWeaponBase, CurrentLevel);
	DOREPLIFETIME(APDWeaponBase, bIsDropped);
	DOREPLIFETIME(APDWeaponBase, WeaponOwner);
	DOREPLIFETIME(APDWeaponBase, ItemID);
	DOREPLIFETIME(APDWeaponBase, WeaponType);
}

void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning,
		TEXT("[PD WeaponPickup] Interact. Weapon=%s Interactor=%s Authority=%d Owner=%s ItemID=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Interactor),
		HasAuthority() ? 1 : 0,
		*GetNameSafe(WeaponOwner),
		*ItemID.ToString());

	if (!HasAuthority()) return;

	if (IsValid(WeaponOwner))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PD WeaponPickup] Failed: already has WeaponOwner. Weapon=%s Owner=%s"),
			*GetNameSafe(this),
			*GetNameSafe(WeaponOwner));
		return;
	}

	APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
	if (!Player)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PD WeaponPickup] Failed: interactor is not player character. Interactor=%s"),
			*GetNameSafe(Interactor));
		return;
	}

	if (ItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PD WeaponPickup] Failed: ItemID is None. Weapon=%s"),
			*GetNameSafe(this));
		OnPickupFailed();
		return;
	}

	UPDInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (!Inventory || !Inventory->AddItemByID(ItemID))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PD WeaponPickup] Failed: AddItemByID failed. Player=%s Inventory=%s ItemID=%s"),
			*GetNameSafe(Player),
			*GetNameSafe(Inventory),
			*ItemID.ToString());
		OnPickupFailed();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[PD WeaponPickup] Succeeded. Weapon=%s Player=%s ItemID=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Player),
		*ItemID.ToString());

	MulticastOnPickedUp();
	ApplyPickedUpPresentation();
	SetLifeSpan(0.2f);
}

void APDWeaponBase::OnRep_Dropped()
{
	if (!IsValid(WeaponOwner))
	{
		SetDropped(bIsDropped);
	}
}

void APDWeaponBase::OnRep_WeaponOwner()
{
	ApplyReplicatedWeaponOwner();
}

void APDWeaponBase::OnRep_WeaponIdentity()
{
	ApplyReplicatedWeaponOwner();
}

void APDWeaponBase::MulticastOnPickedUp_Implementation()
{
	ApplyPickedUpPresentation();
}

void APDWeaponBase::ApplyPickedUpPresentation()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	if (WeaponMesh)
	{
		WeaponMesh->SetVisibility(false, true);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (PickupCollision)
	{
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickupCollision->SetGenerateOverlapEvents(false);
	}
}

void APDWeaponBase::ApplyReplicatedWeaponOwner()
{
	if (!IsValid(WeaponOwner))
	{
		return;
	}

	SetActorEnableCollision(false);
	if (WeaponMesh)
	{
		WeaponMesh->SetVisibility(true, true);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (PickupCollision)
	{
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickupCollision->SetGenerateOverlapEvents(false);
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(WeaponOwner))
	{
		PlayerCharacter->AttachActorToWeaponSocket(this);
		const bool bShouldBeVisible =
			PlayerCharacter->GetReplicatedWeaponType() == WeaponType &&
			PlayerCharacter->GetCurrentSlot() == PlayerCharacter->GetSlotForWeaponType(WeaponType);
		SetActorHiddenInGame(!bShouldBeVisible);
		return;
	}

	if (APDCharacterBase* Character = Cast<APDCharacterBase>(WeaponOwner))
	{
		Character->AttachActorToWeaponSocket(this);
		SetActorHiddenInGame(false);
	}
}

void APDWeaponBase::Fire_Implementation() {}
void APDWeaponBase::Reload_Implementation() {}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	WeaponOwner = NewOwner;
	SetOwner(NewOwner);
	WeaponMesh->SetSimulatePhysics(false);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bIsDropped = false;
	ApplyReplicatedWeaponOwner();
	ForceNetUpdate();


	if (UAbilitySystemComponent* ASCComp = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewOwner))
	{
		FGameplayCueParameters Params;
		Params.Location = WeaponMesh->GetComponentLocation();
		Params.TargetAttachComponent = WeaponMesh;
		Params.SourceObject = this;
		Params.EffectCauser = this;
		Params.Instigator = NewOwner;
		ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_Equip, Params);
	}
}

void APDWeaponBase::OnUnequip_Implementation()
{
	WeaponOwner = nullptr;
	ForceNetUpdate();
}

void APDWeaponBase::UpgradeLevel()
{
	SetLevel(CurrentLevel + 1);
}

void APDWeaponBase::SetLevel(int32 NewLevel)
{
	CurrentLevel = FMath::Clamp(NewLevel, 1, LevelStats.Num());
	OnWeaponLevelChanged.Broadcast(this, CurrentLevel);
}

void APDWeaponBase::SetDropped(bool bDropped)
{
	bIsDropped = bDropped;

	if (bDropped)
	{
		WeaponOwner = nullptr;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		if (WeaponMesh)
		{
			WeaponMesh->SetVisibility(true, true);
		}
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		WeaponMesh->SetSimulatePhysics(true);
	}
	else
	{
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
	}

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

const FWeaponLevelStats& APDWeaponBase::GetCurrentStats() const
{
	static FWeaponLevelStats DefaultStats;
	if (LevelStats.IsEmpty()) return DefaultStats;
	return LevelStats[FMath::Clamp(CurrentLevel - 1, 0, LevelStats.Num() - 1)];
}

FVector APDWeaponBase::GetAimDirectionFromOwner(const FVector& StartLocation) const
{
	if (!IsValid(WeaponOwner)) return FVector::ForwardVector;

	if (UPDWeaponComponent* Comp = WeaponOwner->FindComponentByClass<UPDWeaponComponent>())
		return Comp->GetAimDirection(StartLocation);

	return WeaponOwner->GetActorForwardVector();
}

UTexture2D* APDWeaponBase::GetUISilhouette() const
{
	return UISilhouette.LoadSynchronous();
}
