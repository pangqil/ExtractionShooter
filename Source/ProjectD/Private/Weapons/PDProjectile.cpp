


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
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionComp->SetGenerateOverlapEvents(true);
    CollisionComp->SetNotifyRigidBodyCollision(true);
    SetRootComponent(CollisionComp);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bInitialVelocityInLocalSpace = false;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->InitialSpeed = InitialSpeed;
    ProjectileMovement->MaxSpeed = InitialSpeed;

    InitialLifeSpan = LifeSpan;
}

void APDProjectile::BeginPlay()
{
    Super::BeginPlay();
    CollisionComp->OnComponentHit.AddDynamic(this, &APDProjectile::OnHit);
    CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APDProjectile::OnOverlap);
}

void APDProjectile::InitProjectile(float InDamage, AActor* InOwner, bool bPenetrate, const FVector& InitialDirection)
{
    Damage = InDamage;
    WeaponOwner = InOwner;
    bCanPenetrate = bPenetrate;

    if (CollisionComp)
    {
        if (InOwner)
        {
            CollisionComp->IgnoreActorWhenMoving(InOwner, true);
        }
        if (AActor* WeaponActor = GetOwner())
        {
            CollisionComp->IgnoreActorWhenMoving(WeaponActor, true);
        }
    }

    if (ProjectileMovement && !InitialDirection.IsNearlyZero())
    {
        const float Speed = ProjectileMovement->InitialSpeed > 0.f ? ProjectileMovement->InitialSpeed : InitialSpeed;
        ProjectileMovement->bInitialVelocityInLocalSpace = false;
        ProjectileMovement->ProjectileGravityScale = 0.f;
        ProjectileMovement->MaxSpeed = FMath::Max(ProjectileMovement->MaxSpeed, Speed);
        ProjectileMovement->Velocity = InitialDirection.GetSafeNormal() * Speed;
        ProjectileMovement->UpdateComponentVelocity();
    }
}

void APDProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    HandleProjectileHit(OtherActor, Hit);
}

void APDProjectile::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    HandleProjectileHit(OtherActor, SweepResult);
}

bool APDProjectile::HandleProjectileHit(AActor* OtherActor, const FHitResult& Hit)
{
    if (!HasAuthority())
    {
        return false;
    }
    if (!OtherActor || OtherActor == this || OtherActor == WeaponOwner.Get() || OtherActor == GetOwner())
    {
        return false;
    }

    if (!OtherActor->Implements<UPDDamageable>())
    {
        Destroy();
        return false;
    }

    FPDDamageInfo DamageInfo;
    DamageInfo.BaseDamage = Damage;
    DamageInfo.Instigator = WeaponOwner;
    DamageInfo.DamageTypeClass = nullptr;
    DamageInfo.HitResult = Hit;

    IPDDamageable::Execute_ApplyDamage(OtherActor, DamageInfo);

    if (bCanPenetrate && PenetrationCount < MaxPenetrationCount)
    {
        ++PenetrationCount;
        if (CollisionComp)
        {
            CollisionComp->IgnoreActorWhenMoving(OtherActor, true);
        }
        return true;
    }

    Destroy();
    return true;
}
