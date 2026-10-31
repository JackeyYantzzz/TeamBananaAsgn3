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

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
    CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyTankBase::OnOverlapBegin);

    TankSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TankSpriteComponent"));
    TankSpriteComponent->SetupAttachment(RootComponent);
}

void AEnemyTankBase::BeginPlay()
{
    Super::BeginPlay();
    ChooseNewRandomDirection();
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
    if (GridSize <= 0.0f)
    {
        return true;
    }

    const FVector Location = GetActorLocation();
    const FVector Center(
        TankGrid::TileCenter(Location.X, GridOrigin.X, GridSize),
        Location.Y,
        TankGrid::TileCenter(Location.Z, GridOrigin.Y, GridSize));

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

    if (!IsInsidePlayArea(NewLocation))
    {
        ChooseNewRandomDirection();
        return;
    }

    FHitResult Hit;
    SetActorLocation(NewLocation, true, &Hit);
    if (Hit.bBlockingHit)
    {
        if (ATankBase* Player = Cast<ATankBase>(Hit.GetActor()))
        {
            Player->TakeHit(this);
            ChooseNewRandomDirection();
        }
        else if (Cast<AEnemyTankBase>(Hit.GetActor()) || !TryTurnAssist(DeltaTime))
        {
            ChooseNewRandomDirection();
        }
    }
}

void AEnemyTankBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (IsActorBeingDestroyed())
    {
        return;
    }

    if (OtherActor && OtherActor != this && OtherActor->IsA<ABulletBase>())
    {
        OtherActor->Destroy();

        Health--;
        if (Health <= 0)
        {
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

    if (NewSprite && TankSpriteComponent->GetSprite() != NewSprite)
    {
        TankSpriteComponent->SetSprite(NewSprite);
    }
}

void AEnemyTankBase::ChooseNewRandomDirection()
{
    static const FVector Directions[] =
    {
        FVector(1.0f, 0.0f, 0.0f),
        FVector(-1.0f, 0.0f, 0.0f),
        FVector(0.0f, 0.0f, 1.0f),
        FVector(0.0f, 0.0f, -1.0f)
    };

    FVector NewDirection = CurrentMoveDirection;
    for (int32 Attempt = 0; Attempt < 8 && NewDirection.Equals(CurrentMoveDirection); ++Attempt)
    {
        NewDirection = Directions[FMath::RandRange(0, 3)];
    }
    CurrentMoveDirection = NewDirection;
    UpdateFacingSprite();

    DirectionChangeTimer = FMath::RandRange(MinDirectionTime, MaxDirectionTime);
}
