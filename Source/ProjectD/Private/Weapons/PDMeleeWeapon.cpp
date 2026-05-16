#include "Weapons/PDMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

APDMeleeWeapon::APDMeleeWeapon()
{
	WeaponType = EWeaponType::None;

	AttackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollision"));
	AttackCollision->SetupAttachment(WeaponMesh);
	AttackCollision->SetBoxExtent(FVector(50.f, 10.f, 10.f));
	AttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AttackCollision->OnComponentBeginOverlap.AddDynamic(
		this, &APDMeleeWeapon::OnAttackOverlap);
}

void APDMeleeWeapon::Fire_Implementation()
{
	if (!bCanFire) return;

	if (AttackMontage && WeaponOwner.IsValid())
	{
		USkeletalMeshComponent* OwnerMesh =
			WeaponOwner->FindComponentByClass<USkeletalMeshComponent>();

		if (OwnerMesh)
		{
			if (UAnimInstance* Inst = OwnerMesh->GetAnimInstance())
				Inst->Montage_Play(AttackMontage);
		}
	}

	bCanFire = false;
	GetWorldTimerManager().SetTimer(
		FireCooldownHandle,
		FTimerDelegate::CreateLambda([this]() { bCanFire = true; }),
		AttackCooldown, false);
}

void APDMeleeWeapon::EnableAttackCollision()
{
	AlreadyHitActors.Empty();
	AttackCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void APDMeleeWeapon::DisableAttackCollision()
{
	AttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AlreadyHitActors.Empty();
}

void APDMeleeWeapon::OnAttackOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;
	if (OtherActor == GetWeaponOwner()) return;
	if (AlreadyHitActors.Contains(OtherActor)) return;

	AlreadyHitActors.Add(OtherActor);
	ApplyDamage(OtherActor, AttackDamage);
}

void APDMeleeWeapon::ApplyDamage(AActor* TargetActor, float DamageAmount)
{
	if (!TargetActor || DamageAmount <= 0.f)
	{
		return;
	}

	UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, nullptr, this, nullptr);
}
