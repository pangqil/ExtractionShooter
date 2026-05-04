#include "Core/PDPlayerController.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

#include "GameplayTag/PDGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Core/PDGameMode.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Input/PDInputComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDRifle.h"

APDPlayerController::APDPlayerController()
{
	bShowMouseCursor=true;
	PrimaryActorTick.bCanEverTick=true;
}

void APDPlayerController::RequestExtraction()
{
	if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GM->RequestExtraction(this);
	}
}

void APDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateAimRotation();
}

void APDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem=
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	UPDInputComponent* PDIC=CastChecked<UPDInputComponent>(InputComponent);
	if (!InputConfig) return;

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Move, 
		ETriggerEvent::Triggered, this, &APDPlayerController::OnMove);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Jump,
	ETriggerEvent::Started, this, &APDPlayerController::OnJump);
	PDIC->BindAbilityActions(InputConfig, this, &APDPlayerController::OnAbilityInputPressed,
		&APDPlayerController::OnAbilityInputReleased);
	// 무기 입력
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Fire,
		ETriggerEvent::Started, this, &APDPlayerController::OnFirePressed);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Fire,
		ETriggerEvent::Completed, this, &APDPlayerController::OnFireReleased);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Reload,
		ETriggerEvent::Started, this, &APDPlayerController::OnReload);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_WeaponSlot1,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot1);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_WeaponSlot2,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot2);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_WeaponSlot3,
		ETriggerEvent::Started, this, &APDPlayerController::OnSwitchSlot3);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_ToggleFireMode,
		ETriggerEvent::Started, this, &APDPlayerController::OnToggleFireMode);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Interact,
		ETriggerEvent::Started, this, &APDPlayerController::OnInteract);
}

void APDPlayerController::OnMove(const struct FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMove 호출됨"));

	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput=Value.Get<FVector2D>();
	ControlledPawn->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	ControlledPawn->AddMovementInput(FVector::RightVector, MoveInput.X);
}

void APDPlayerController::OnJump()
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}


void APDPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayTagContainer TagContainer(InputTag);
		ASC->CancelAbilities(&TagContainer);
	}
}


void APDPlayerController::UpdateAimRotation()
{
	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit)) return;

	FVector AimDirection=Hit.Location-ControlledPawn->GetActorLocation();
	AimDirection.Z=0.f;

	if (!AimDirection.IsNearlyZero())
	{
		ControlledPawn->SetActorRotation(AimDirection.Rotation());
	}
}

APDPlayerCharacter* APDPlayerController::GetPlayerCharacter() const
{
	return Cast<APDPlayerCharacter>(GetPawn());
}

APDWeaponBase* APDPlayerController::GetCurrentWeapon() const
{
	if (APDPlayerCharacter* PC = GetPlayerCharacter())
		return PC->GetCurrentWeapon();
	return nullptr;
}

// 무기 입력 핸들러

void APDPlayerController::OnFirePressed()
{
	APDWeaponBase* CurWeapon = GetCurrentWeapon();
	if (!CurWeapon) return;

	if (APDRifle* Rifle = Cast<APDRifle>(CurWeapon))
		Rifle->StartFire();
	else
		CurWeapon->Fire();
}

void APDPlayerController::OnFireReleased()
{
	if (APDRifle* Rifle = Cast<APDRifle>(GetCurrentWeapon()))
		Rifle->StopFire();
}

void APDPlayerController::OnReload()
{
	if (APDWeaponBase* CurWeapon = GetCurrentWeapon())
		CurWeapon->Reload();
}

void APDPlayerController::OnSwitchSlot1()
{
	if (APDPlayerCharacter* PC = GetPlayerCharacter())
		PC->SwitchToSlot(EWeaponSlot::Slot1_Rifle);
}

void APDPlayerController::OnSwitchSlot2()
{
	if (APDPlayerCharacter* PC = GetPlayerCharacter())
		PC->SwitchToSlot(EWeaponSlot::Slot2_Shotgun);
}

void APDPlayerController::OnSwitchSlot3()
{
	if (APDPlayerCharacter* PC = GetPlayerCharacter())
		PC->SwitchToSlot(EWeaponSlot::Slot3_Sniper);
}

void APDPlayerController::OnToggleFireMode()
{
	if (APDRifle* Rifle = Cast<APDRifle>(GetCurrentWeapon()))
		Rifle->ToggleFireMode();
}

void APDPlayerController::OnInteract()
{
	APDPlayerCharacter* PC = GetPlayerCharacter();
	if (!PC) return;

	TArray<AActor*> OverlappingActors;
	PC->GetOverlappingActors(OverlappingActors, APDWeaponBase::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->Implements<UPDInteractable>())
		{
			IPDInteractable::Execute_Interact(Actor, PC);
			break;
		}
	}
}