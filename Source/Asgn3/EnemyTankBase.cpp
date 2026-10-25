#include "EnemyTankBase.h"
#include "TankBase.h"
#include "BulletBase.h"
#include "EnemySpawner.h"
#include "TankGridUtils.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Math/UnrealMathUtility.h"

AEnemyTankBase::AEnemyTankBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // Initialize collision box as the root component
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
    // Block solid actors (wood crates, other tanks) while still receiving overlap events from bullets
    CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyTankBase::OnOverlapBegin);

    // Initialize sprite component and attach it to the root
    TankSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TankSpriteComponent"));
    TankSpriteComponent->SetupAttachment(RootComponent);
}

void AEnemyTankBase::BeginPlay()
{
    Super::BeginPlay();
    ChooseNewRandomDirection(); // Initial random movement direction
}

void AEnemyTankBase::SetPlayArea(FVector2D InMinXZ, FVector2D InMaxXZ)
{
    PlayAreaMin = InMinXZ;
    PlayAreaMax = InMaxXZ;
    bHasPlayArea = true;
}

void AEnemyTankBase::SetGrid(FVector2D InOriginXZ, float InGridSize)
{
    GridOrigin = InOriginXZ;
    GridSize = InGridSize;
}

bool AEnemyTankBase::SnapToTileCenterIfClose(float DeltaTime)
{
    // Without a grid the enemy may turn anywhere
    if (GridSize <= 0.0f)
    {
        return true;
    }

    const FVector Location = GetActorLocation();
    const FVector Center(
        TankGrid::TileCenter(Location.X, GridOrigin.X, GridSize),
        Location.Y,
        TankGrid::TileCenter(Location.Z, GridOrigin.Y, GridSize));

    // Close enough to the tile centre (within one frame of movement): snap and allow a turn
    if (FVector::Dist(Location, Center) <= MoveSpeed * DeltaTime + 1.0f)
    {
        SetActorLocation(Center, true);
        return true;
    }
    return false;
}

bool AEnemyTankBase::TryTurnAssist(float DeltaTime)
{
    if (GridSize <= 0.0f)
    {
        return false;
    }

    const bool bMovingX = FMath::Abs(CurrentMoveDirection.X) > 0.9f;
    const FVector Location = GetActorLocation();
    const double Coord  = bMovingX ? Location.Z : Location.X;
    const double Origin = bMovingX ? GridOrigin.Y : GridOrigin.X;
    const double Diff = TankGrid::TileCenter(Coord, Origin, GridSize) - Coord;

    if (FMath::Abs(Diff) > KINDA_SMALL_NUMBER && FMath::Abs(Diff) <= TurnAssistDistance)
    {
        const double Step = FMath::Sign(Diff) * FMath::Min(FMath::Abs(Diff), (double)(MoveSpeed * DeltaTime));
        const FVector Nudge = bMovingX ? FVector(0.0, 0.0, Step) : FVector(Step, 0.0, 0.0);
        FHitResult NudgeHit;
        SetActorLocation(Location + Nudge, true, &NudgeHit);
        return !NudgeHit.bBlockingHit;
    }
    return false;
}

bool AEnemyTankBase::IsInsidePlayArea(const FVector& Location) const
{
    if (!bHasPlayArea)
    {
        return true;
    }
    return Location.X >= PlayAreaMin.X && Location.X <= PlayAreaMax.X
        && Location.Z >= PlayAreaMin.Y && Location.Z <= PlayAreaMax.Y;
}

void AEnemyTankBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Change direction every few seconds, like the classic Battle City enemies
    // On a grid the enemy only turns once it reaches a tile centre, so it stays lined up with corridors
    DirectionChangeTimer -= DeltaTime;
    if (DirectionChangeTimer <= 0.0f && SnapToTileCenterIfClose(DeltaTime))
    {
        ChooseNewRandomDirection();
    }

    if (CurrentMoveDirection.IsZero())
    {
        return;
    }

    const FVector NewLocation = GetActorLocation() + (CurrentMoveDirection * MoveSpeed * DeltaTime);

    // Turn around instead of leaving the map
    if (!IsInsidePlayArea(NewLocation))
    {
        ChooseNewRandomDirection();
        return;
    }

    // Move with sweep so walls, crates and other tanks block the enemy
    FHitResult Hit;
    SetActorLocation(NewLocation, true, &Hit);
    if (Hit.bBlockingHit)
    {
        // Ramming the player costs the player 1 health (the player has a short invincibility window)
        if (ATankBase* Player = Cast<ATankBase>(Hit.GetActor()))
        {
            Player->TakeHit(this);
            ChooseNewRandomDirection();
        }
        else if (Cast<AEnemyTankBase>(Hit.GetActor()) || !TryTurnAssist(DeltaTime))
        {
            // Blocked by another tank, or by a wall we cannot slide past: pick another direction
            ChooseNewRandomDirection();
        }
    }
}

void AEnemyTankBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // Ignore further hits once destroyed (two bullets of a double shot can arrive in the same frame)
    if (IsActorBeingDestroyed())
    {
        return;
    }

    // Only player bullets damage enemies
    if (OtherActor && OtherActor != this && OtherActor->IsA<ABulletBase>())
    {
        OtherActor->Destroy();

        Health--;
        if (Health <= 0)
        {
            // Count the kill (every few kills the spawner drops a pickup)
            if (AEnemySpawner* Spawner = Cast<AEnemySpawner>(GetOwner()))
            {
                Spawner->NotifyEnemyKilled(this);
            }
            Destroy();
        }
    }
}

void AEnemyTankBase::UpdateFacingSprite()
{
    if (!TankSpriteComponent)
    {
        return;
    }

    UPaperSprite* NewSprite = nullptr;
    if (FMath::Abs(CurrentMoveDirection.X) >= FMath::Abs(CurrentMoveDirection.Z))
    {
        NewSprite = CurrentMoveDirection.X >= 0.0f ? SpriteRight : SpriteLeft;
    }
    else
    {
        NewSprite = CurrentMoveDirection.Z >= 0.0f ? SpriteUp : SpriteDown;
    }

    // Keep the current sprite if that direction has none assigned
    if (NewSprite && TankSpriteComponent->GetSprite() != NewSprite)
    {
        TankSpriteComponent->SetSprite(NewSprite);
    }
}

void AEnemyTankBase::ChooseNewRandomDirection()
{
    // Battle City style: only move up / down / left / right on the X-Z plane
    static const FVector Directions[] =
    {
        FVector(1.0f, 0.0f, 0.0f),
        FVector(-1.0f, 0.0f, 0.0f),
        FVector(0.0f, 0.0f, 1.0f),
        FVector(0.0f, 0.0f, -1.0f)
    };

    // Prefer a direction different from the current one so a blocked tank actually turns
    FVector NewDirection = CurrentMoveDirection;
    for (int32 Attempt = 0; Attempt < 8 && NewDirection.Equals(CurrentMoveDirection); ++Attempt)
    {
        NewDirection = Directions[FMath::RandRange(0, 3)];
    }
    CurrentMoveDirection = NewDirection;
    UpdateFacingSprite();

    DirectionChangeTimer = FMath::RandRange(MinDirectionTime, MaxDirectionTime);
}
