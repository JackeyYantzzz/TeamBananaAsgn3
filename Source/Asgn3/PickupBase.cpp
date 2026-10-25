#include "PickupBase.h"
#include "TankBase.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"

APickupBase::APickupBase()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(14.0f, 14.0f, 14.0f));
    // Never blocks anything, only reports overlaps
    CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnOverlapBegin);

    PickupSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PickupSprite"));
    PickupSprite->SetupAttachment(RootComponent);
    PickupSprite->SetCollisionProfileName(TEXT("NoCollision"));
    // The pickup images are 50x50 pixels at 1 pixel per unit; scale them to roughly one tank (28 units)
    PickupSprite->SetRelativeScale3D(FVector(0.56f));
}

void APickupBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // Only the player can collect pickups (enemies and bullets pass through them)
    ATankBase* Player = Cast<ATankBase>(OtherActor);
    if (IsActorBeingDestroyed() || !Player || Player->IsDead())
    {
        return;
    }

    switch (PickupType)
    {
    case EPickupType::Health:
        Player->Heal(HealAmount);
        break;
    case EPickupType::DoubleShot:
        Player->ActivateDoubleShot(DoubleShotDuration);
        break;
    }

    Destroy();
}
