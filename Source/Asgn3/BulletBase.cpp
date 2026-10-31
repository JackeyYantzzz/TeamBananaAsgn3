#include "BulletBase.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"

ABulletBase::ABulletBase()
{
    PrimaryActorTick.bCanEverTick = true;

    Tags.Add(FName(TEXT("Bullet")));

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(10.0f, 10.0f, 10.0f));
    CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    BulletSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BulletSpriteComponent"));
    BulletSpriteComponent->SetupAttachment(RootComponent);
}

void ABulletBase::BeginPlay()
{
    Super::BeginPlay();

    SetLifeSpan(LifeSpanTimer);
}

void ABulletBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector MoveDelta = GetActorForwardVector() * BulletSpeed * DeltaTime;
    AddActorWorldOffset(MoveDelta, true);
}