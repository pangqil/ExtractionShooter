


#include "Weapons/PDProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/PDDamageable.h"

APDProjectile::APDProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
    CollisionComp->InitSphereRadius(5.f);
    CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComp->SetNotifyRigidBodyCollision(true);
    SetRootComponent(CollisionComp);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->InitialSpeed = InitialSpeed;
    ProjectileMovement->MaxSpeed = InitialSpeed;

    InitialLifeSpan = LifeSpan;
}

void APDProjectile::BeginPlay()
{
    Super::BeginPlay();
    CollisionComp->OnComponentHit.AddDynamic(this, &APDProjectile::OnHit);
}

void APDProjectile::InitProjectile(float InDamage, AActor* InOwner, bool bPenetrate)
{
    Damage = InDamage;
    WeaponOwner = InOwner;
    bCanPenetrate = bPenetrate;
}

void APDProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!HasAuthority()) return;
    if (!OtherActor || OtherActor == this || OtherActor == WeaponOwner.Get()) return;


    if (!OtherActor->Implements<UPDDamageable>()) return;


    FPDDamageInfo DamageInfo;
    DamageInfo.BaseDamage = Damage;
    DamageInfo.Instigator = WeaponOwner;
    DamageInfo.DamageTypeClass = nullptr;
    DamageInfo.HitResult = Hit;

    IPDDamageable::Execute_ApplyDamage(OtherActor, DamageInfo);


    if (bCanPenetrate && PenetrationCount < MaxPenetrationCount)
        ++PenetrationCount;
    else
        Destroy();
}
