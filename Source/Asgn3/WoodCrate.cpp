#include "WoodCrate.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"
#include "BulletBase.h"

AWoodCrate::AWoodCrate()
{
    PrimaryActorTick.bCanEverTick = false;

    // Setup root collision box to physically block tanks
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));

    // Set collision profile to block tanks and dynamic objects
    CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    // Ensure overlap events are enabled so bullets can trigger overlap while being blocked
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWoodCrate::OnOverlapBegin);

    // Setup sprite component for the wooden crate appearance
    CrateSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("CrateSpriteComponent"));
    CrateSpriteComponent->SetupAttachment(RootComponent);
}

void AWoodCrate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // Ensure we have a valid other actor and it's not self
    if (OtherActor && OtherActor != this)
    {
        // Only bullets can destroy the crate; tanks are physically blocked instead
        if (OtherActor->IsA<ABulletBase>() || OtherActor->ActorHasTag(TEXT("Bullet")))
        {
            // Reduce health and destroy the crate when health is depleted
            Health--;
            if (Health <= 0)
            {
                Destroy();
            }

            // Destroy the bullet upon impact
            OtherActor->Destroy();
        }
    }
}