#include "BulletBase.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"

ABulletBase::ABulletBase()
{
    // Enable Tick function every frame
    PrimaryActorTick.bCanEverTick = true;

    // Tag used by AWoodCrate to recognise bullets (only bullets can destroy crates)
    Tags.Add(FName(TEXT("Bullet")));

    // Setup root collision box suitable for 2D projectile collision
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(10.0f, 10.0f, 10.0f));
    CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // Setup bullet sprite component attached to the root component
    BulletSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BulletSpriteComponent"));
    BulletSpriteComponent->SetupAttachment(RootComponent);
}

void ABulletBase::BeginPlay()
{
    Super::BeginPlay();

    // Automatically destroy the bullet actor after a set time to free up memory
    SetLifeSpan(LifeSpanTimer);
}

void ABulletBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Move forward along the bullet's heading direction every frame with collision sweep enabled
    FVector MoveDelta = GetActorForwardVector() * BulletSpeed * DeltaTime;
    AddActorWorldOffset(MoveDelta, true);
}