#include "WoodCrate.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"
#include "BulletBase.h"

AWoodCrate::AWoodCrate()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));

    CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWoodCrate::OnOverlapBegin);

    CrateSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("CrateSpriteComponent"));
    CrateSpriteComponent->SetupAttachment(RootComponent);
}

void AWoodCrate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        if (OtherActor->IsA<ABulletBase>() || OtherActor->ActorHasTag(TEXT("Bullet")))
        {
            Health--;
            if (Health <= 0)
            {
                Destroy();
            }

            OtherActor->Destroy();
        }
    }
}