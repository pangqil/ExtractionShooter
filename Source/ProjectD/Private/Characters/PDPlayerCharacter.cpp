#include "Characters/PDPlayerCharacter.h"

#include "AttributeSet/PDAttributeSet.h"
﻿#include "Characters/PDPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Component/PDVisionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Weapons/PDWeaponBase.h"


APDPlayerCharacter::APDPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	VisionComponent=CreateDefaultSubobject<UPDVisionComponent>(TEXT("VisionComponent"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	// 무기 슬롯 초기화
	WeaponSlots.SetNum(3);
}

void APDPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
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
void APDPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// 무기 관리
void APDPlayerCharacter::PickupWeapon(APDWeaponBase* Weapon)
{
    if (!Weapon) return;

    EWeaponSlot TargetSlot = GetSlotForWeaponType(Weapon->GetWeaponType());
    if (TargetSlot == EWeaponSlot::None) return;

    int32 Idx = static_cast<int32>(TargetSlot);

    if (WeaponSlots[Idx])
        WeaponSlots[Idx]->OnUnequip();

    WeaponSlots[Idx] = Weapon;
    OnWeaponPickedUp.Broadcast(Weapon);

    if (CurrentSlot == EWeaponSlot::None)
        SwitchToSlot(TargetSlot);
}

void APDPlayerCharacter::SwitchToSlot(EWeaponSlot Slot)
{
    if (Slot == CurrentSlot) return;

    int32 Idx = static_cast<int32>(Slot);
    if (!WeaponSlots.IsValidIndex(Idx) || !WeaponSlots[Idx]) return;

    if (APDWeaponBase* CurWeapon = GetCurrentWeapon())
    {
        CurWeapon->OnUnequip();
        CurWeapon->SetActorHiddenInGame(true);
    }

    CurrentSlot = Slot;
    APDWeaponBase* NewWeapon = WeaponSlots[Idx];
    NewWeapon->OnEquip(this);
    NewWeapon->SetActorHiddenInGame(false);

    // CharacterBase 소켓에 붙이기
    AttachActorToWeaponSocket(NewWeapon);

    OnWeaponSwapped.Broadcast(NewWeapon, Slot);
}

void APDPlayerCharacter::DropCurrentWeapon()
{
    APDWeaponBase* CurWeapon = GetCurrentWeapon();
    if (!CurWeapon) return;

    CurWeapon->OnUnequip();
    CurWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    WeaponSlots[static_cast<int32>(CurrentSlot)] = nullptr;
    CurrentSlot = EWeaponSlot::None;
}

void APDPlayerCharacter::HandleDeath(AActor* Killer)
{
    if (APDWeaponBase* CurWeapon = GetCurrentWeapon())
        CurWeapon->OnUnequip();

    Super::HandleDeath(Killer);
}

// Getter,내부 함수

APDWeaponBase* APDPlayerCharacter::GetCurrentWeapon() const
{
    if (CurrentSlot == EWeaponSlot::None) return nullptr;
    int32 Idx = static_cast<int32>(CurrentSlot);
    return WeaponSlots.IsValidIndex(Idx) ? WeaponSlots[Idx].Get() : nullptr;
}

APDWeaponBase* APDPlayerCharacter::GetWeaponInSlot(EWeaponSlot Slot) const
{
    int32 Idx = static_cast<int32>(Slot);
    return WeaponSlots.IsValidIndex(Idx) ? WeaponSlots[Idx].Get() : nullptr;
}

EWeaponSlot APDPlayerCharacter::GetSlotForWeaponType(EWeaponType Type) const
{
    switch (Type)
    {
    case EWeaponType::Rifle:   return EWeaponSlot::Slot1_Rifle;
    case EWeaponType::Shotgun: return EWeaponSlot::Slot2_Shotgun;
    case EWeaponType::Sniper:  return EWeaponSlot::Slot3_Sniper;
    default:                   return EWeaponSlot::None;
    }
}