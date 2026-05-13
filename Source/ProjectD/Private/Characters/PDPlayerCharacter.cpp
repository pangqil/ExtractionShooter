#include "Characters/PDPlayerCharacter.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Component/PDVisionComponent.h"
#include "Component/PDInteractionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Items/PDEquipmentComponent.h"
#include "Weapons/PDWeaponBase.h"

APDPlayerCharacter::APDPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch=false;
	bUseControllerRotationYaw=false;
	bUseControllerRotationRoll=false;

	GetCharacterMovement()->bOrientRotationToMovement=true;
	GetCharacterMovement()->RotationRate=FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane=true;
	GetCharacterMovement()->bSnapToPlaneAtStart=true;

	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength=800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest=false;

	TopDownCameraComponent=CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation=false;

	VisionComponent=CreateDefaultSubobject<UPDVisionComponent>(TEXT("VisionComponent"));
	InteractionComponent=CreateDefaultSubobject<UPDInteractionComponent>(TEXT("InteractionComponent"));
	QuickSlotComponent=CreateDefaultSubobject<UPDQuickSlotComponent>(TEXT("QuickSlotComponent"));
	EquipmentComponent=CreateDefaultSubobject<UPDEquipmentComponent>(TEXT("EquipmentComponent"));

	PrimaryActorTick.bCanEverTick=true;
	PrimaryActorTick.bStartWithTickEnabled=true;
	WeaponSlots.SetNum(4);

	// Player 팀. AI 의 GetTeamAttitudeTowards 에서 적대 판정의 기준이 됨.
	TeamID = 1;
}

void APDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	 if (ASC)
        {
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Rifle,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Shotgun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Sniper,  EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Pistol,  EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
        }
	LinkDefaultAnimLayer();

}

void APDPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APDPlayerCharacter::InitAbilitySystem()
{
	Super::InitAbilitySystem();

	auto ApplyInfiniteGE=[&](TSubclassOf<UGameplayEffect> GEClass)
	{
		if (!GEClass) return;
		FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
		if (Spec.IsValid())
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	};

	if (UPDVisionComponent* Vision=FindComponentByClass<UPDVisionComponent>())
	{
		Vision->BindToAttributeSet(ASC);
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &APDPlayerCharacter::OnStaminaChanged);

	ApplyInfiniteGE(HungerDecayEffectClass);
	ApplyInfiniteGE(ThirstDecayEffectClass);
}

void APDPlayerCharacter::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!AttributeSet||!VisionComponent) return;
	const float MaxStamina=AttributeSet->GetMaxStamina();
	if (MaxStamina<=0.f) return;
	VisionComponent->UpdateStaminaScale(Data.NewValue/MaxStamina);
}

void APDPlayerCharacter::HandleDeath(AActor* Killer)
{
	if (APDWeaponBase* CurWeapon=GetCurrentWeapon())
		CurWeapon->OnUnequip();
	Super::HandleDeath(Killer);
}

void APDPlayerCharacter::PickupWeapon(APDWeaponBase* Weapon)
{
	if (!Weapon) return;
	EWeaponSlot TargetSlot=GetSlotForWeaponType(Weapon->GetWeaponType());
	if (TargetSlot==EWeaponSlot::None) return;

	int32 Idx=static_cast<int32>(TargetSlot);
	// 슬롯이 차있을 때는 호출자(Interact/AutoEquip)에서 인벤토리 경로로 우회해야 한다.
	if (WeaponSlots[Idx]) return;

	WeaponSlots[Idx]=Weapon;
	Weapon->OnEquip(this);           
	Weapon->SetActorHiddenInGame(true);
	AttachActorToWeaponSocket(Weapon);
	OnWeaponPickedUp.Broadcast(Weapon);

	if (CurrentSlot==EWeaponSlot::None)
		SwitchToSlot(TargetSlot);
}

void APDPlayerCharacter::SwitchToSlot(EWeaponSlot Slot)
{
	if (Slot==CurrentSlot) return;

	int32 Idx=static_cast<int32>(Slot);
	if (!WeaponSlots.IsValidIndex(Idx)||!WeaponSlots[Idx]) return;

	if (APDWeaponBase* CurWeapon=GetCurrentWeapon())
	{
		CurWeapon->OnUnequip();
		CurWeapon->SetActorHiddenInGame(true);
	}

	CurrentSlot=Slot;
	APDWeaponBase* NewWeapon=WeaponSlots[Idx];
	NewWeapon->OnEquip(this);
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Rifle);
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Shotgun);
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Sniper);
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Pistol);
		ASC->AddLooseGameplayTag(NewWeapon->GetWeaponTypeTag());
	}
	NewWeapon->SetActorHiddenInGame(false);
	AttachActorToWeaponSocket(NewWeapon);
	OnWeaponSwapped.Broadcast(NewWeapon, Slot);
}

void APDPlayerCharacter::DropCurrentWeapon()
{
	APDWeaponBase* CurWeapon=GetCurrentWeapon();
	if (!CurWeapon) return;

	CurWeapon->OnUnequip();
	CurWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	WeaponSlots[static_cast<int32>(CurrentSlot)]=nullptr;
	CurrentSlot=EWeaponSlot::None;
	ASC->RemoveLooseGameplayTag(CurWeapon->GetWeaponTypeTag());
}

APDWeaponBase* APDPlayerCharacter::GetCurrentWeapon() const
{
	if (CurrentSlot==EWeaponSlot::None) return nullptr;
	int32 Idx=static_cast<int32>(CurrentSlot);
	return WeaponSlots.IsValidIndex(Idx) ? WeaponSlots[Idx].Get() : nullptr;
}

APDWeaponBase* APDPlayerCharacter::GetWeaponInSlot(EWeaponSlot Slot) const
{
	int32 Idx=static_cast<int32>(Slot);
	return WeaponSlots.IsValidIndex(Idx) ? WeaponSlots[Idx].Get() : nullptr;
}

EWeaponSlot APDPlayerCharacter::GetSlotForWeaponType(EWeaponType Type) const
{
	switch (Type)
	{
	case EWeaponType::Rifle:   return EWeaponSlot::Slot1_Rifle;
	case EWeaponType::Shotgun: return EWeaponSlot::Slot2_Shotgun;
	case EWeaponType::Sniper:  return EWeaponSlot::Slot3_Sniper;
	case EWeaponType::Pistol:  return EWeaponSlot::Slot4_Pistol;
	default:                   return EWeaponSlot::None;
	}
}

bool APDPlayerCharacter::TryAutoEquipWeaponItem(const FPDItemData& ItemData)
{
	if (!ItemData.WeaponClass) return false;

	const EWeaponSlot TargetSlot = GetSlotForWeaponType(ItemData.WeaponType);
	if (TargetSlot == EWeaponSlot::None) return false;

	// 이미 해당 슬롯에 무기가 있으면 자동 장착 안 함(호출자가 인벤토리로 보내야 함).
	if (GetWeaponInSlot(TargetSlot)) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDWeaponBase* SpawnedWeapon = World->SpawnActor<APDWeaponBase>(
		ItemData.WeaponClass, GetActorTransform(), SpawnParams);
	if (!SpawnedWeapon) return false;

	PickupWeapon(SpawnedWeapon);
	return true;
}


bool APDPlayerCharacter::RemoveEquippedWeaponItem(const FPDItemData& ItemData, bool bDestroyWeaponActor)
{
	const EWeaponSlot TargetSlot = GetSlotForWeaponType(ItemData.WeaponType);
	if (TargetSlot == EWeaponSlot::None)
	{
		return false;
	}

	const int32 SlotIndex = static_cast<int32>(TargetSlot);
	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex])
	{
		return false;
	}

	APDWeaponBase* WeaponToRemove = WeaponSlots[SlotIndex];
	WeaponToRemove->OnUnequip();
	WeaponToRemove->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	WeaponToRemove->SetActorHiddenInGame(true);

	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(WeaponToRemove->GetWeaponTypeTag());
	}

	WeaponSlots[SlotIndex] = nullptr;
	if (CurrentSlot == TargetSlot)
	{
		CurrentSlot = EWeaponSlot::None;
	}

	if (bDestroyWeaponActor)
	{
		WeaponToRemove->Destroy();
	}

	return true;
}

void APDPlayerCharacter::TryInteract()
{
	if (InteractionComponent)
	{
		InteractionComponent->Interact();
	}
}

void APDPlayerCharacter::OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount==0) return;
	APDWeaponBase* CurWeapon=GetCurrentWeapon();
	if (!IsValid(CurWeapon)) return;
	TSubclassOf<UAnimInstance> LayerClass=CurWeapon->GetWeaponAnimLayerClass();
	if (!LayerClass) { LinkDefaultAnimLayer(); return; }
	if (USkeletalMeshComponent* SkelMesh=GetMesh())
		SkelMesh->LinkAnimClassLayers(LayerClass);
}

void APDPlayerCharacter::LinkDefaultAnimLayer()
{
	if (!DefaultAnimLayerClass) return;
	if (USkeletalMeshComponent* SkelMesh=GetMesh())
		SkelMesh->LinkAnimClassLayers(DefaultAnimLayerClass);
}
